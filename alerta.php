<style>
body,html{
background: white;
}
table{
margin-left:auto; 
margin-right:auto;
border: solid 1px;
border-collapse: collapse;
}
table td,table th{
padding-left: 0.6rem;
padding-right: 0.6rem;
text-align: center;
border: solid 1px;
}
@media (max-width: 1200px) {
  table td,table th{
    font-size: 0.9rem;
    padding-left: 0.1rem;
    padding-right: 0.1rem;
 }
}
div {
width: 100%;
text-align: center;
} 
</style>
<?php


$mysqli = new mysqli("127.0.0.1", "USUARIO", "CLAVE", "DB");

function select_fechas($fecha_actual){
  global $mysqli;
  
  $sql = "SELECT 
	  DATE_FORMAT(lecturas_sensor.fecha,'%Y-%m-%d')  fechas
	  FROM lecturas_sensor
	  GROUP BY fechas DESC";
  $change = 'onchange="document.getElementById(\'form\').submit()"';
  $select = '<form id="form" action="alerta.php?h=1" method="POST">'.
            'Filtrar por fecha: <select '.$change.' id="f" name="f">';

  $mysqli->query($sql);
  if ( $result = $mysqli->query($sql) ) {

     while( $row = $result->fetch_object() ){

        if ( trim($fecha_actual,"'") == $row->fechas )
          $s = 'selected="selected"';
        else
          $s = '';

       $select .= '<option '.$s.'>'.$row->fechas.'</option>';
     }
  }
  $select .= '</select></form>';
  return $select;
}


// Si recibo valores de tensión solo numéricos (ev. exploits)
if ( isset($_POST['v'],$_POST['u']) &&
     is_numeric($_POST['v']) &&
     is_numeric($_POST['u'])
                            ){
   // ENVÍO EMAIL CON ALERTA DE TENSIÓN BAJA SI 
   // TENGO EL SLIDER EN LA POSICIÓN ON:

   if (isset($_POST['m']) && $_POST['m'] == 1){

	function generateMessageID(){
	  return sprintf(
	    "<%s.%s@%s>",
	    base_convert(microtime(), 10, 36),
	    base_convert(bin2hex(openssl_random_pseudo_bytes(8)), 16, 36),
	    '<usar FQDN>' 
	  );
	}

	include("Mail.php");
	include('Mail/mime.php');

	$recipients = "destinatario@.......";

	$headers["From"] = "no-reply@.......";

	$headers["To"] = $recipients;
	$headers["MIME-Version"] = "1.0";
	$headers["Message-ID"] =  generateMessageID();
	$headers["Date"] = date("r");
	$headers["Return-Path"] =  "no-reply@.......";
	$headers["X-Priority"] =  "1";//1 = High, 3 = Normal, 5 = Low
	$headers["X-Mailer"] =  "PHP/" . phpversion();

	$headers["Subject"] = "Alerta de tension baja";

	$html = '<!DOCTYPE html>
	<html lang="es">
	<body>
	<p>
	Alerta de tensión por debajo del umbral establecido.<br/><br/>
	Lectura actual:<br/><br/><b>'.$_POST['v'].'&nbsp;V</b>&nbsp;&nbsp;('.date("d/m/Y H:i:s").'hs) 
	</p>
	</body>
	</html>';
	$crlf = "\n";

	$mime = new Mail_mime(array('eol' => $crlf));

	// Cuerpo del email
	$mime->setTXTBody(strip_tags($text));
	$mime->setHTMLBody($html);

	$body = $mime->get(array('html_charset' => 'utf-8', 'text_charset' => 'utf-8', 'eol' => $crlf));
	$headers = $mime->headers($headers);

	 $params["host"] = "ssl://.......";
	 $params["port"] = "465";
	 $params["auth"] = true;
	 $params["username"] = USUARIO;
	 $params["password"] = CONTRASEÑA;

	 // Envío objeto email usando el método Mail::factory
	 $mail_object =& Mail::factory("smtp", $params);
	 $mail_object->send($recipients, $headers, $body);
	// print_r($envio_correcto); 
        }


	$sql = "INSERT INTO lecturas_sensor (tension,umbral,fecha) ".
               "VALUES ('".$_POST['v']."','".$_POST['u']."',NOW())";

	$mysqli->query($sql);
	$mysqli->close();
}	

// SI SOLICITO UNA TABLA CON LOS VALORES ALMACENADOS EN DB:
	
if (isset($_POST['h']) || isset($_GET['h'])) {

    if (isset($_POST['f']) && !empty($_POST['f'])){
      $fecha_filtrada = "'".$_POST['f']."'";
    }else{
      $fecha_filtrada = "(SELECT DATE_FORMAT(MAX(tbl.fecha),'%Y-%m-%d') ". 
                        " FROM lecturas_sensor tbl)";
    }

    $sql = " SELECT tension,umbral,DATE_FORMAT(fecha,'%d/%m/%Y') AS fecha, ".
           " DATE_FORMAT(fecha,'%H:%i:%s') AS hora ".
           " FROM lecturas_sensor WHERE DATE_FORMAT(lecturas_sensor.fecha,'%Y-%m-%d') = ".$fecha_filtrada.
           " ORDER BY lecturas_sensor.fecha DESC";
    if ($result = $mysqli->query($sql)) {

        $html = '<html><head><title>Histórico de lecturas de tensión</title><body>'.
                '<div>'.
                select_fechas($fecha_filtrada).
                '<table>'.
                '<tr><th>Tensión</th><th>Umbral</th><th>Estado</th><th>Fecha</th><th>Tiempo</th></tr>';  

        while($obj = $result->fetch_object()){
            $c = '';
            $alarm = 'Normal';
            if ( (float)$obj->tension < (float)$obj->umbral ){
               $c = 'style="background: red; font-weight: bold; color: #FFF;"';
               $alarm = 'Alerta';
            }
          
            $html .= '<tr><td>'.$obj->tension.'</td>'.
                         '<td>'.$obj->umbral.'</td>'.
                         '<td '.$c.'>'.$alarm.'</td>'.
                         '<td>'.$obj->fecha.'</td>'.
                         '<td>'.$obj->hora.' hs</td>'.
                     '</tr>';
        }
        $html .= '</table></div></body></html>';
        echo $html; 
    } 
}
?>
