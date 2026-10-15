#include "scene.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

namespace corncob::graphics {

namespace {

view_parameters constexpr cockpit_view{
  .bounds{
    .left{8},
    .top{4},
    .right{631},
    .bottom{199},
  },
  .focal_x{300},
  .focal_y{200},
  .centre_x{319},
  .centre_y{101},
  .rear{false},
};
view_parameters constexpr rear_view{
  .bounds{
    .left{168},
    .top{254},
    .right{471},
    .bottom{344},
  },
  .focal_x{150},
  .focal_y{100},
  .centre_x{320},
  .centre_y{299},
  .rear{true},
};
view_parameters constexpr full_view{
  .bounds{
    .left{8},
    .top{4},
    .right{631},
    .bottom{345},
  },
  .focal_x{300},
  .focal_y{200},
  .centre_x{319},
  .centre_y{174},
  .rear{false},
};

maths::fixed_vector in_view(maths::fixed_vector const &point, view_parameters const &view) noexcept {
  /// Rear projection reverses the forward and lateral axes while retaining height
  return {view.rear ? maths::negate(point.x) : point.x, view.rear ? maths::negate(point.y) : point.y, point.z};
}

std::optional<screen_point> project(maths::fixed_vector const &point, view_parameters const &view) noexcept {
  /// The engine looks along +X and projects negated Y/Z with separate horizontal and vertical focal lengths
  if(point.x <= view.focal_x) return std::nullopt;
  return screen_point{
    maths::word(static_cast<int32_t>(maths::negate(point.y)) * view.focal_x / point.x + view.centre_x),
    maths::word(static_cast<int32_t>(maths::negate(point.z)) * view.focal_y / point.x + view.centre_y),
  };
}

maths::fixed_vector intersection(maths::fixed_vector const &outside, maths::fixed_vector const &inside, int const plane) noexcept {
  /// CHOPLIN interpolates at the focal plane, then nudges its depth one unit forward
  int const distance{plane - outside.x};
  int const depth{inside.x - outside.x};
  return {
    static_cast<int16_t>(plane + 1),
    maths::word(outside.y + static_cast<int32_t>(maths::word(inside.y - outside.y)) * distance / depth),
    maths::word(outside.z + static_cast<int32_t>(maths::word(inside.z - outside.z)) * distance / depth),
  };
}

maths::fixed_vector model_vertex(maths::fixed_vector const &point, maths::rotation const &matrix,
                                 maths::fixed_vector const &origin, unsigned int const reduction) noexcept {
  /// Distance detail reduces both the model and its relative origin before the fast word transform
  maths::fixed_vector const reduced{
    static_cast<int16_t>(point.x >> reduction),
    static_cast<int16_t>(point.y >> reduction),
    static_cast<int16_t>(point.z >> reduction),
  };
  auto const rotated{maths::transform_vertex(matrix, reduced)};
  return {
    maths::word(rotated.x + origin.x),
    maths::word(rotated.y + origin.y),
    maths::word(rotated.z + origin.z),
  };
}

unsigned int model_octant(maths::fixed_vector const &relative, maths::rotation const &inverse) noexcept {
  /// DRROBJ packs the signs of three separately rounded dot products in the model's own axes
  unsigned int result{0};
  for(unsigned int axis{0}; axis != 3; ++axis) {
    int32_t sum{0};
    for(unsigned int component{0}; component != 3; ++component) sum += maths::multiply(relative[component], inverse.rows[component][axis]);
    result = (result << 1) | (maths::word(sum) < 0 ? 1u : 0u);
  }
  return result;
}

struct visible_object {
  world::placed_object const *object;
  resources::model_address model;
  unsigned int tier;
  unsigned int reduction;
  uint32_t distance;
  maths::fixed_vector relative{0, 0, 0};
};

std::optional<visible_object> visibility(world::placed_object const &object, resources::object_definition const &definition,
                                       maths::world_position const &observer) noexcept {
  /// Translate DISTEVAL's three coordinate scales and D3DOBJ's original model breakpoints
  maths::world_position const delta{object.position - observer};
  int64_t maximum{0};
  for(unsigned int axis{0}; axis != 3; ++axis) maximum = std::max(maximum, std::abs(static_cast<int64_t>(delta[axis])));
  if(maximum >= 8'388'608) return std::nullopt;
  unsigned int const tier{maximum < 32'768 ? 0u : maximum < 524'288 ? 1u : 2u};
  unsigned int const distance_shift{tier * 4};
  uint32_t distance{0};
  for(unsigned int axis{0}; axis != 3; ++axis) {
    int32_t const component{maths::word(delta[axis] >> distance_shift)};
    distance += static_cast<uint32_t>(component * component);
  }
  unsigned int reduction{0};
  auto address{definition.models.near};
  if(tier == 0 && distance > definition.limits.near_squared) {
    reduction = 4;
    address = definition.models.middle;
  } else if(tier == 1) {
    reduction = distance > definition.limits.middle_squared ? 8u : 4u;
    address = reduction == 4 ? definition.models.middle : definition.models.far;
  } else if(tier == 2) {
    if(distance > definition.limits.far_squared) return std::nullopt;
    reduction = 8;
    address = definition.models.far;
  }
  if(object.type == world::object_type::aircraft && (object.flags & 0x2000u) != 0 && reduction != 8) address = resources::geometry_bank::experimental_model;
  return visible_object{
    .object{&object},
    .model{address},
    .tier{tier},
    .reduction{reduction},
    .distance{distance},
    .relative{
      maths::word(delta.x >> (reduction + 1)),
      maths::word(delta.y >> (reduction + 1)),
      maths::word(delta.z >> (reduction + 1)),
    },
  };
}

} // anonymous namespace

void scene_renderer::set_palette(world::theatre const &theatre, maths::world_position const &position) {
  /// Restore EGA colours, then unpack the theatre's native six-bit sky and ground RGB values
  static std::array<render::rgba_pixel, 16> constexpr ega{
    render::rgba_pixel{0, 0, 0}, render::rgba_pixel{0, 0, 170}, render::rgba_pixel{0, 170, 0}, render::rgba_pixel{0, 170, 170},
    render::rgba_pixel{170, 0, 0}, render::rgba_pixel{170, 0, 170}, render::rgba_pixel{170, 85, 0}, render::rgba_pixel{170, 170, 170},
    render::rgba_pixel{85, 85, 85}, render::rgba_pixel{85, 85, 255}, render::rgba_pixel{85, 255, 85}, render::rgba_pixel{85, 255, 255},
    render::rgba_pixel{255, 85, 85}, render::rgba_pixel{255, 85, 255}, render::rgba_pixel{255, 255, 85}, render::rgba_pixel{255, 255, 255},
  };
  palette = ega;
  int const tile_x{std::clamp((position.x - world::theatre::origin) / world::theatre::tile_size, 0, 7)};
  int const tile_y{std::clamp((position.y - world::theatre::origin) / world::theatre::tile_size, 0, 7)};
  uint64_t packed{0};
  for(auto const byte : theatre.tiles[static_cast<unsigned int>(tile_y) * world::theatre::columns + static_cast<unsigned int>(tile_x)].packed_colours) packed = (packed << 8) | byte;
  if(packed != 0) {
    auto component{[packed](unsigned int const shift){
      unsigned int const value{static_cast<unsigned int>((packed >> shift) & 63u)};
      return static_cast<uint8_t>((value << 2) | (value >> 4));
    }};
    palette[2] = {
      .red{component(14)},
      .green{component(8)},
      .blue{component(2)},
    };
    palette[11] = {
      .red{component(32)},
      .green{component(26)},
      .blue{component(20)},
    };
  }
}

void scene_renderer::background(view_parameters const &view, maths::rotation const &rotation) {
  /// Classify each pixel ray against the world horizon without losing sky or ground at steep attitudes
  int const direction{view.rear ? -1 : 1};
  for(int y{view.bounds.top}; y != view.bounds.bottom; ++y) {
    for(int x{view.bounds.left}; x != view.bounds.right; ++x) {
      int64_t const height{
        static_cast<int64_t>(rotation.rows[0].z) * view.focal_x * view.focal_y * direction
        - static_cast<int64_t>(rotation.rows[1].z) * (x - view.centre_x) * view.focal_y * direction
        - static_cast<int64_t>(rotation.rows[2].z) * (y - view.centre_y) * view.focal_x
      };
      indexed.pixels[static_cast<unsigned int>(y) * render::screen_layout::width + static_cast<unsigned int>(x)] = height >= 0 ? 11 : 2;
    }
  }
}

void scene_renderer::draw_model(resources::model_address const address, unsigned int const octant, maths::rotation const &matrix,
                                maths::fixed_vector const &origin, unsigned int const reduction, std::span<view_parameters const> const views) {
  /// Execute the authored primitive sequence; software rendering uses no depth buffer or polygon sorting
  for(auto const &command : geometry.commands(address, octant)) {
    std::visit([this, &matrix, &origin, reduction, views](auto const &shape){
      using shape_type = std::remove_cvref_t<decltype(shape)>;
      for(auto const &view : views) {
        raster draw{indexed, view.bounds, random};
        if constexpr(std::is_same_v<shape_type, resources::polygon>) {
          std::vector<maths::fixed_vector> transformed;
          for(auto const &vertex : shape.vertices) transformed.push_back(in_view(model_vertex(vertex, matrix, origin, reduction), view));
          std::vector<maths::fixed_vector> clipped;
          if(transformed.empty()) continue;
          for(unsigned int i{0}; i != transformed.size(); ++i) {
            auto const &a{transformed[i]}, &b{transformed[(i + 1) % transformed.size()]};
            bool const inside_a{a.x > view.focal_x}, inside_b{b.x > view.focal_x};
            if(inside_a) clipped.push_back(maths::fixed_vector{a});
            if(inside_a != inside_b) clipped.push_back(inside_a ? intersection(b, a, view.focal_x) : intersection(a, b, view.focal_x));
          }
          std::vector<screen_point> projected;
          for(auto const &vertex : clipped) {
            if(auto const point{project(vertex, view)}) projected.push_back(screen_point{*point});
          }
          if(projected.size() >= 3) {
            draw.polygon(projected, geometry.colour(shape.colour));
            ++statistics.polygons;
          }
        } else if constexpr(std::is_same_v<shape_type, resources::wireframe>) {
          for(auto const &line : shape.lines) {
            for(unsigned int i{1}; i < line.vertices.size(); ++i) {
              auto a{in_view(model_vertex(line.vertices[i - 1], matrix, origin, reduction), view)};
              auto b{in_view(model_vertex(line.vertices[i], matrix, origin, reduction), view)};
              if(a.x <= view.focal_x && b.x <= view.focal_x) continue;
              if(a.x <= view.focal_x) a = intersection(a, b, view.focal_x);
              else if(b.x <= view.focal_x) b = intersection(b, a, view.focal_x);
              auto const first{project(a, view)}, last{project(b, view)};
              if(first && last) {
                draw.line(screen_point{*first}, screen_point{*last}, geometry.colour(line.colour));
                ++statistics.lines;
              }
            }
          }
        } else if constexpr(std::is_same_v<shape_type, resources::point_cloud>) {
          for(auto const &point : shape.points) {
            if(auto const projected{project(in_view(model_vertex(point.position, matrix, origin, reduction), view), view)}) draw.point(*projected, geometry.colour(point.colour));
          }
        } else if constexpr(std::is_same_v<shape_type, resources::disc_cloud>) {
          for(auto const &disc : shape.discs) {
            auto const centre{in_view(model_vertex(disc.position, matrix, origin, reduction), view)};
            if(auto const projected{project(centre, view)}) {
              int const diameter{static_cast<int>(disc.diameter >> reduction) * view.focal_x / centre.x};
              draw.disc(*projected, diameter, geometry.colour(disc.colour));
              ++statistics.discs;
            }
          }
        }
      }
    }, geometry.drawing(command.primitive_index));
  }
}

scene_renderer::scene_renderer(resources::geometry_bank &geometry, resources::cockpit_mask const &cockpit) :
  geometry{geometry},
  cockpit{cockpit} {
  /// Keep immutable world resources outside the renderer and own only frame-local display state
}

render_statistics scene_renderer::render(world::theatre const &theatre, camera const &observer, unsigned int const frame,
                                        uint16_t const timer, render::framebuffer &output) {
  /// Render the original world through fixed-point transforms into a 640x350 indexed software surface
  statistics = {};
  set_palette(theatre, observer.position);
  std::ranges::fill(indexed.pixels, 0);
  if(observer.cockpit) {
    for(unsigned int i{0}; i != indexed.pixels.size(); ++i) indexed.pixels[i] = (cockpit[i / 8] & (0x80u >> (i & 7))) != 0 ? 7 : 0;
  }
  std::array<view_parameters, 2> const view_storage{observer.cockpit ? cockpit_view : full_view, rear_view};
  std::span<view_parameters const> const views{view_storage.data(), observer.cockpit && observer.rear_view ? 2u : 1u};
  // EPAGE writes byte-wide frame strips before the scenery overwrites each view
  auto strip{[this](unsigned int const first_byte, unsigned int const bytes, unsigned int const rows){
    unsigned int constexpr bytes_per_row{render::screen_layout::width / 8};
    for(unsigned int row{0}; row != rows; ++row) {
      unsigned int const first{(first_byte + row * bytes_per_row) * 8};
      unsigned int const end{std::min(first + bytes * 8, static_cast<unsigned int>(indexed.pixels.size()))};
      if(first < end) std::fill(indexed.pixels.begin() + first, indexed.pixels.begin() + end, 7);
    }
  }};
  auto const &main_view{view_storage.front().bounds};
  unsigned int const first_row{static_cast<unsigned int>(main_view.top) * 80};
  unsigned int const main_height{static_cast<unsigned int>(main_view.bottom - main_view.top)};
  strip(first_row, 1, main_height);
  strip(first_row + 78, 1, main_height);
  strip(0, 80, 8);
  strip(static_cast<unsigned int>(main_view.bottom) * 80, 80, 8);
  if(observer.cockpit && observer.rear_view) {
    strip(19'940, 40, 5);
    strip(27'540, 40, 5);
    strip(20'340, 1, 91);
    strip(20'379, 1, 91);
  }
  auto const rotation{maths::make_rotation(observer.attitude)};
  for(auto const &view : views) background(view, rotation);

  maths::angle constexpr sun_angle{8192};
  maths::fixed_vector const sun_relative{0, maths::multiply(maths::negate(maths::sine(sun_angle)), 1000), maths::multiply(maths::cosine(sun_angle), 1000)};
  auto const sun_origin{maths::transform(rotation, sun_relative)};
  draw_model(resources::geometry_bank::sun_model, 0, rotation, sun_origin, 0, views);

  std::vector<visible_object> visible;
  for(auto const &object : theatre.objects) {
    if(!object.has(world::object_flag::present)) continue;
    if(auto item{visibility(object, geometry.definition(object.type), observer.position)}) visible.push_back(std::move(*item));
  }
  std::ranges::stable_sort(visible, [](visible_object const &lhs, visible_object const &rhs){
    return lhs.tier != rhs.tier ? lhs.tier > rhs.tier : lhs.distance > rhs.distance;
  });
  uint8_t const shared_random{static_cast<uint8_t>(random.next() % 15)};
  for(auto const &item : visible) {
    auto const &object{*item.object};
    geometry.prepare_colours(object, frame, timer, shared_random, false);
    auto const inverse{maths::make_inverse_rotation(object.attitude)};
    auto const matrix{item.tier < geometry.definition(object.type).rotate_below_tier ? maths::compose(rotation, inverse) : rotation};
    auto const origin{maths::transform(rotation, item.relative)};
    draw_model(item.model, model_octant(item.relative, inverse), matrix, origin, item.reduction, views);
    ++statistics.objects;
  }
  for(unsigned int i{0}; i != output.pixels.size(); ++i) output.pixels[i] = palette[indexed.pixels[i] & 15u];
  return statistics;
}

} // namespace corncob::graphics
