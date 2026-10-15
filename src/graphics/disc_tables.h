#pragma once

#include <array>
#include <cstdint>

namespace corncob::graphics {

struct disc_pattern_row {
  int y;
  uint16_t mask;
};

struct disc_pattern {
  unsigned int first_row;
  unsigned int row_count;
};

// original small-disc masks and CSQRT table, byte-verified against the 3.42 executable
inline std::array<disc_pattern, 17> constexpr small_discs{
  disc_pattern{
    .first_row{0},
    .row_count{1},
  },
  disc_pattern{
    .first_row{1},
    .row_count{1},
  },
  disc_pattern{
    .first_row{2},
    .row_count{2},
  },
  disc_pattern{
    .first_row{4},
    .row_count{3},
  },
  disc_pattern{
    .first_row{7},
    .row_count{3},
  },
  disc_pattern{
    .first_row{10},
    .row_count{4},
  },
  disc_pattern{
    .first_row{14},
    .row_count{5},
  },
  disc_pattern{
    .first_row{19},
    .row_count{6},
  },
  disc_pattern{
    .first_row{25},
    .row_count{7},
  },
  disc_pattern{
    .first_row{32},
    .row_count{7},
  },
  disc_pattern{
    .first_row{39},
    .row_count{7},
  },
  disc_pattern{
    .first_row{46},
    .row_count{8},
  },
  disc_pattern{
    .first_row{54},
    .row_count{8},
  },
  disc_pattern{
    .first_row{62},
    .row_count{9},
  },
  disc_pattern{
    .first_row{71},
    .row_count{10},
  },
  disc_pattern{
    .first_row{81},
    .row_count{11},
  },
  disc_pattern{
    .first_row{92},
    .row_count{11},
  },
};

inline std::array<disc_pattern_row, 103> constexpr small_disc_rows{
  disc_pattern_row{
    .y{0},
    .mask{0x0080},
  },
  disc_pattern_row{
    .y{0},
    .mask{0x0080},
  },
  disc_pattern_row{
    .y{-1},
    .mask{0x0080},
  },
  disc_pattern_row{
    .y{0},
    .mask{0x00c0},
  },
  disc_pattern_row{
    .y{-1},
    .mask{0x0180},
  },
  disc_pattern_row{
    .y{0},
    .mask{0x01c0},
  },
  disc_pattern_row{
    .y{1},
    .mask{0x0080},
  },
  disc_pattern_row{
    .y{-1},
    .mask{0x0180},
  },
  disc_pattern_row{
    .y{0},
    .mask{0x03c0},
  },
  disc_pattern_row{
    .y{1},
    .mask{0x0180},
  },
  disc_pattern_row{
    .y{-2},
    .mask{0x01c0},
  },
  disc_pattern_row{
    .y{-1},
    .mask{0x03e0},
  },
  disc_pattern_row{
    .y{0},
    .mask{0x03e0},
  },
  disc_pattern_row{
    .y{1},
    .mask{0x01c0},
  },
  disc_pattern_row{
    .y{-2},
    .mask{0x03c0},
  },
  disc_pattern_row{
    .y{-1},
    .mask{0x07e0},
  },
  disc_pattern_row{
    .y{0},
    .mask{0x07e0},
  },
  disc_pattern_row{
    .y{1},
    .mask{0x07e0},
  },
  disc_pattern_row{
    .y{2},
    .mask{0x03c0},
  },
  disc_pattern_row{
    .y{-3},
    .mask{0x01c0},
  },
  disc_pattern_row{
    .y{-2},
    .mask{0x07f0},
  },
  disc_pattern_row{
    .y{-1},
    .mask{0x07f0},
  },
  disc_pattern_row{
    .y{0},
    .mask{0x07f0},
  },
  disc_pattern_row{
    .y{1},
    .mask{0x07f0},
  },
  disc_pattern_row{
    .y{2},
    .mask{0x01c0},
  },
  disc_pattern_row{
    .y{-3},
    .mask{0x03c0},
  },
  disc_pattern_row{
    .y{-2},
    .mask{0x07e0},
  },
  disc_pattern_row{
    .y{-1},
    .mask{0x0ff0},
  },
  disc_pattern_row{
    .y{0},
    .mask{0x0ff0},
  },
  disc_pattern_row{
    .y{1},
    .mask{0x0ff0},
  },
  disc_pattern_row{
    .y{2},
    .mask{0x07e0},
  },
  disc_pattern_row{
    .y{3},
    .mask{0x03c0},
  },
  disc_pattern_row{
    .y{-3},
    .mask{0x03e0},
  },
  disc_pattern_row{
    .y{-2},
    .mask{0x07f0},
  },
  disc_pattern_row{
    .y{-1},
    .mask{0x0ff8},
  },
  disc_pattern_row{
    .y{0},
    .mask{0x0ff8},
  },
  disc_pattern_row{
    .y{3},
    .mask{0x03e0},
  },
  disc_pattern_row{
    .y{2},
    .mask{0x07f0},
  },
  disc_pattern_row{
    .y{1},
    .mask{0x0ff8},
  },
  disc_pattern_row{
    .y{-3},
    .mask{0x07e0},
  },
  disc_pattern_row{
    .y{-2},
    .mask{0x0ff0},
  },
  disc_pattern_row{
    .y{-1},
    .mask{0x1ff8},
  },
  disc_pattern_row{
    .y{0},
    .mask{0x1ff8},
  },
  disc_pattern_row{
    .y{3},
    .mask{0x07e0},
  },
  disc_pattern_row{
    .y{2},
    .mask{0x0ff0},
  },
  disc_pattern_row{
    .y{1},
    .mask{0x1ff8},
  },
  disc_pattern_row{
    .y{-4},
    .mask{0x03e0},
  },
  disc_pattern_row{
    .y{-3},
    .mask{0x0ff8},
  },
  disc_pattern_row{
    .y{-2},
    .mask{0x1ffc},
  },
  disc_pattern_row{
    .y{-1},
    .mask{0x1ffc},
  },
  disc_pattern_row{
    .y{0},
    .mask{0x1ffc},
  },
  disc_pattern_row{
    .y{1},
    .mask{0x1ffc},
  },
  disc_pattern_row{
    .y{2},
    .mask{0x0ff8},
  },
  disc_pattern_row{
    .y{3},
    .mask{0x03e0},
  },
  disc_pattern_row{
    .y{-4},
    .mask{0x07e0},
  },
  disc_pattern_row{
    .y{-3},
    .mask{0x1ff8},
  },
  disc_pattern_row{
    .y{-2},
    .mask{0x3ffc},
  },
  disc_pattern_row{
    .y{-1},
    .mask{0x3ffc},
  },
  disc_pattern_row{
    .y{0},
    .mask{0x3ffc},
  },
  disc_pattern_row{
    .y{1},
    .mask{0x3ffc},
  },
  disc_pattern_row{
    .y{2},
    .mask{0x1ff8},
  },
  disc_pattern_row{
    .y{3},
    .mask{0x07e0},
  },
  disc_pattern_row{
    .y{-4},
    .mask{0x03e0},
  },
  disc_pattern_row{
    .y{-3},
    .mask{0x0ff8},
  },
  disc_pattern_row{
    .y{-2},
    .mask{0x1ffc},
  },
  disc_pattern_row{
    .y{-1},
    .mask{0x3ffe},
  },
  disc_pattern_row{
    .y{0},
    .mask{0x3ffe},
  },
  disc_pattern_row{
    .y{1},
    .mask{0x3ffe},
  },
  disc_pattern_row{
    .y{2},
    .mask{0x1ffc},
  },
  disc_pattern_row{
    .y{3},
    .mask{0x0ff8},
  },
  disc_pattern_row{
    .y{4},
    .mask{0x03e0},
  },
  disc_pattern_row{
    .y{-5},
    .mask{0x07e0},
  },
  disc_pattern_row{
    .y{-4},
    .mask{0x1ff8},
  },
  disc_pattern_row{
    .y{-3},
    .mask{0x3ffc},
  },
  disc_pattern_row{
    .y{-2},
    .mask{0x7ffe},
  },
  disc_pattern_row{
    .y{-1},
    .mask{0x7ffe},
  },
  disc_pattern_row{
    .y{0},
    .mask{0x7ffe},
  },
  disc_pattern_row{
    .y{1},
    .mask{0x7ffe},
  },
  disc_pattern_row{
    .y{2},
    .mask{0x3ffc},
  },
  disc_pattern_row{
    .y{3},
    .mask{0x1ff8},
  },
  disc_pattern_row{
    .y{4},
    .mask{0x07e0},
  },
  disc_pattern_row{
    .y{-5},
    .mask{0x03e0},
  },
  disc_pattern_row{
    .y{-4},
    .mask{0x0ff8},
  },
  disc_pattern_row{
    .y{-3},
    .mask{0x3ffe},
  },
  disc_pattern_row{
    .y{-2},
    .mask{0x3ffe},
  },
  disc_pattern_row{
    .y{-1},
    .mask{0x7fff},
  },
  disc_pattern_row{
    .y{0},
    .mask{0x7fff},
  },
  disc_pattern_row{
    .y{1},
    .mask{0x7fff},
  },
  disc_pattern_row{
    .y{5},
    .mask{0x03e0},
  },
  disc_pattern_row{
    .y{4},
    .mask{0x0ff8},
  },
  disc_pattern_row{
    .y{3},
    .mask{0x3ffe},
  },
  disc_pattern_row{
    .y{2},
    .mask{0x3ffe},
  },
  disc_pattern_row{
    .y{-5},
    .mask{0x07e0},
  },
  disc_pattern_row{
    .y{-4},
    .mask{0x1ff8},
  },
  disc_pattern_row{
    .y{-3},
    .mask{0x7ffe},
  },
  disc_pattern_row{
    .y{-2},
    .mask{0x7ffe},
  },
  disc_pattern_row{
    .y{-1},
    .mask{0xffff},
  },
  disc_pattern_row{
    .y{0},
    .mask{0xffff},
  },
  disc_pattern_row{
    .y{1},
    .mask{0xffff},
  },
  disc_pattern_row{
    .y{5},
    .mask{0x07e0},
  },
  disc_pattern_row{
    .y{4},
    .mask{0x1ff8},
  },
  disc_pattern_row{
    .y{3},
    .mask{0x7ffe},
  },
  disc_pattern_row{
    .y{2},
    .mask{0x7ffe},
  },
};

inline std::array<uint16_t, 1024> constexpr disc_square_root{
  1474, 1473, 1473, 1473, 1473, 1473, 1473, 1473, 1473, 1473, 1473, 1473,
  1473, 1473, 1473, 1473, 1473, 1473, 1473, 1473, 1473, 1473, 1473, 1473,
  1473, 1473, 1473, 1473, 1473, 1473, 1473, 1473, 1473, 1473, 1473, 1473,
  1473, 1473, 1472, 1472, 1472, 1472, 1472, 1472, 1472, 1472, 1472, 1472,
  1472, 1472, 1472, 1472, 1472, 1472, 1471, 1471, 1471, 1471, 1471, 1471,
  1471, 1471, 1471, 1471, 1471, 1471, 1470, 1470, 1470, 1470, 1470, 1470,
  1470, 1470, 1470, 1470, 1469, 1469, 1469, 1469, 1469, 1469, 1469, 1469,
  1469, 1468, 1468, 1468, 1468, 1468, 1468, 1468, 1468, 1467, 1467, 1467,
  1467, 1467, 1467, 1467, 1466, 1466, 1466, 1466, 1466, 1466, 1466, 1465,
  1465, 1465, 1465, 1465, 1465, 1464, 1464, 1464, 1464, 1464, 1464, 1464,
  1463, 1463, 1463, 1463, 1463, 1462, 1462, 1462, 1462, 1462, 1462, 1461,
  1461, 1461, 1461, 1461, 1460, 1460, 1460, 1460, 1460, 1459, 1459, 1459,
  1459, 1459, 1458, 1458, 1458, 1458, 1458, 1457, 1457, 1457, 1457, 1457,
  1456, 1456, 1456, 1456, 1455, 1455, 1455, 1455, 1454, 1454, 1454, 1454,
  1454, 1453, 1453, 1453, 1453, 1452, 1452, 1452, 1452, 1451, 1451, 1451,
  1451, 1450, 1450, 1450, 1450, 1449, 1449, 1449, 1448, 1448, 1448, 1448,
  1447, 1447, 1447, 1447, 1446, 1446, 1446, 1445, 1445, 1445, 1445, 1444,
  1444, 1444, 1443, 1443, 1443, 1442, 1442, 1442, 1442, 1441, 1441, 1441,
  1440, 1440, 1440, 1439, 1439, 1439, 1438, 1438, 1438, 1437, 1437, 1437,
  1436, 1436, 1436, 1436, 1435, 1435, 1434, 1434, 1434, 1433, 1433, 1433,
  1432, 1432, 1432, 1431, 1431, 1431, 1430, 1430, 1430, 1429, 1429, 1429,
  1428, 1428, 1427, 1427, 1427, 1426, 1426, 1426, 1425, 1425, 1424, 1424,
  1424, 1423, 1423, 1423, 1422, 1422, 1421, 1421, 1421, 1420, 1420, 1419,
  1419, 1419, 1418, 1418, 1417, 1417, 1417, 1416, 1416, 1415, 1415, 1414,
  1414, 1414, 1413, 1413, 1412, 1412, 1411, 1411, 1411, 1410, 1410, 1409,
  1409, 1408, 1408, 1407, 1407, 1407, 1406, 1406, 1405, 1405, 1404, 1404,
  1403, 1403, 1402, 1402, 1402, 1401, 1401, 1400, 1400, 1399, 1399, 1398,
  1398, 1397, 1397, 1396, 1396, 1395, 1395, 1394, 1394, 1393, 1393, 1392,
  1392, 1391, 1391, 1390, 1390, 1389, 1389, 1388, 1388, 1387, 1387, 1386,
  1386, 1385, 1385, 1384, 1384, 1383, 1383, 1382, 1382, 1381, 1380, 1380,
  1379, 1379, 1378, 1378, 1377, 1377, 1376, 1376, 1375, 1374, 1374, 1373,
  1373, 1372, 1372, 1371, 1371, 1370, 1369, 1369, 1368, 1368, 1367, 1367,
  1366, 1365, 1365, 1364, 1364, 1363, 1362, 1362, 1361, 1361, 1360, 1359,
  1359, 1358, 1358, 1357, 1356, 1356, 1355, 1355, 1354, 1353, 1353, 1352,
  1351, 1351, 1350, 1350, 1349, 1348, 1348, 1347, 1346, 1346, 1345, 1344,
  1344, 1343, 1343, 1342, 1341, 1341, 1340, 1339, 1339, 1338, 1337, 1337,
  1336, 1335, 1335, 1334, 1333, 1333, 1332, 1331, 1330, 1330, 1329, 1328,
  1328, 1327, 1326, 1326, 1325, 1324, 1324, 1323, 1322, 1321, 1321, 1320,
  1319, 1319, 1318, 1317, 1316, 1316, 1315, 1314, 1313, 1313, 1312, 1311,
  1311, 1310, 1309, 1308, 1308, 1307, 1306, 1305, 1305, 1304, 1303, 1302,
  1302, 1301, 1300, 1299, 1298, 1298, 1297, 1296, 1295, 1295, 1294, 1293,
  1292, 1291, 1291, 1290, 1289, 1288, 1287, 1287, 1286, 1285, 1284, 1283,
  1283, 1282, 1281, 1280, 1279, 1279, 1278, 1277, 1276, 1275, 1274, 1274,
  1273, 1272, 1271, 1270, 1269, 1268, 1268, 1267, 1266, 1265, 1264, 1263,
  1262, 1262, 1261, 1260, 1259, 1258, 1257, 1256, 1255, 1255, 1254, 1253,
  1252, 1251, 1250, 1249, 1248, 1247, 1246, 1246, 1245, 1244, 1243, 1242,
  1241, 1240, 1239, 1238, 1237, 1236, 1235, 1234, 1234, 1233, 1232, 1231,
  1230, 1229, 1228, 1227, 1226, 1225, 1224, 1223, 1222, 1221, 1220, 1219,
  1218, 1217, 1216, 1215, 1214, 1213, 1212, 1211, 1210, 1209, 1208, 1207,
  1206, 1205, 1204, 1203, 1202, 1201, 1200, 1199, 1198, 1197, 1196, 1195,
  1194, 1193, 1192, 1191, 1190, 1189, 1188, 1187, 1186, 1184, 1183, 1182,
  1181, 1180, 1179, 1178, 1177, 1176, 1175, 1174, 1173, 1172, 1170, 1169,
  1168, 1167, 1166, 1165, 1164, 1163, 1162, 1160, 1159, 1158, 1157, 1156,
  1155, 1154, 1152, 1151, 1150, 1149, 1148, 1147, 1146, 1144, 1143, 1142,
  1141, 1140, 1138, 1137, 1136, 1135, 1134, 1133, 1131, 1130, 1129, 1128,
  1126, 1125, 1124, 1123, 1122, 1120, 1119, 1118, 1117, 1115, 1114, 1113,
  1112, 1110, 1109, 1108, 1107, 1105, 1104, 1103, 1102, 1100, 1099, 1098,
  1096, 1095, 1094, 1093, 1091, 1090, 1089, 1087, 1086, 1085, 1083, 1082,
  1081, 1079, 1078, 1077, 1075, 1074, 1073, 1071, 1070, 1069, 1067, 1066,
  1064, 1063, 1062, 1060, 1059, 1057, 1056, 1055, 1053, 1052, 1050, 1049,
  1048, 1046, 1045, 1043, 1042, 1040, 1039, 1038, 1036, 1035, 1033, 1032,
  1030, 1029, 1027, 1026, 1024, 1023, 1021, 1020, 1018, 1017, 1015, 1014,
  1012, 1011, 1009, 1008, 1006, 1005, 1003, 1002, 1000, 998, 997, 995,
  994, 992, 991, 989, 987, 986, 984, 983, 981, 979, 978, 976,
  974, 973, 971, 970, 968, 966, 965, 963, 961, 960, 958, 956,
  955, 953, 951, 949, 948, 946, 944, 943, 941, 939, 937, 936,
  934, 932, 930, 929, 927, 925, 923, 921, 920, 918, 916, 914,
  912, 911, 909, 907, 905, 903, 901, 899, 898, 896, 894, 892,
  890, 888, 886, 884, 882, 880, 879, 877, 875, 873, 871, 869,
  867, 865, 863, 861, 859, 857, 855, 853, 851, 849, 847, 845,
  843, 840, 838, 836, 834, 832, 830, 828, 826, 824, 821, 819,
  817, 815, 813, 811, 808, 806, 804, 802, 800, 797, 795, 793,
  791, 788, 786, 784, 782, 779, 777, 775, 772, 770, 768, 765,
  763, 760, 758, 756, 753, 751, 748, 746, 743, 741, 739, 736,
  734, 731, 728, 726, 723, 721, 718, 716, 713, 710, 708, 705,
  703, 700, 697, 695, 692, 689, 686, 684, 681, 678, 675, 673,
  670, 667, 664, 661, 658, 655, 653, 650, 647, 644, 641, 638,
  635, 632, 629, 626, 623, 620, 616, 613, 610, 607, 604, 601,
  597, 594, 591, 587, 584, 581, 577, 574, 571, 567, 564, 560,
  557, 553, 550, 546, 542, 539, 535, 531, 528, 524, 520, 516,
  512, 509, 505, 501, 497, 493, 489, 484, 480, 476, 472, 468,
  463, 459, 454, 450, 445, 441, 436, 432, 427, 422, 417, 412,
  407, 402, 397, 392, 387, 382, 376, 371, 365, 359, 354, 348,
  342, 336, 330, 323, 317, 310, 303, 296, 289, 282, 275, 267,
  259, 251, 242, 234, 224, 215, 205, 194, 183, 172, 159, 145,
  130, 112, 92, 65,
};

} // namespace corncob::graphics
