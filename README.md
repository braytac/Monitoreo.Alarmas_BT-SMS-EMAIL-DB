# Monitoreo_Alarmas_BT-SMS-EMAIL-DB
Envío de valores de tensión y señales de alarma con EDU-CIAA por BT, alertas por SMS email y almacenamiento en DB

Estos archivos y directorio deberán estar en el directori raíz del firmware_v3 de la CIAA

- Cargar MonitorTension_BluetoothCIAA.aia en APP Inventor
- Subir alerta.php a un servidor apache/PHP/MYSQL, ejecutar la consulta en alguna DB para crear la tabla:
    CREATE TABLE lecturas_sensor  (
      `id` int(11) NOT NULL AUTO_INCREMENT,
      `umbral` varchar(5) CHARACTER SET latin1 COLLATE latin1_swedish_ci NULL DEFAULT NULL,
      `tension` varchar(5) CHARACTER SET latin1 COLLATE latin1_swedish_ci NULL DEFAULT '',
      `fecha` timestamp(0) NULL DEFAULT NULL,
      PRIMARY KEY (`id`) USING BTREE
    ) ENGINE = InnoDB AUTO_INCREMENT = 769 CHARACTER SET = latin1 COLLATE = latin1_swedish_ci ROW_FORMAT = Dynamic;
- Compilar y escribir firmware en la CIAA (make && make download)
