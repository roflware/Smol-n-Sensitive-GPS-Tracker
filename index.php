<?php

  // Remember to replace all variables that state: UPDATE_TO_YOURS

        function read_last_line ($file_path){

                $line = '';
                $f = fopen($file_path, 'r');
                $cursor = -1;

                fseek($f, $cursor, SEEK_END);
                $char = fgetc($f);

                while ($char === "\n" || $char === "\r") {
                    fseek($f, $cursor--, SEEK_END);
                    $char = fgetc($f);
                }

                while ($char !== false && $char !== "\n" && $char !== "\r") {
                    $line = $char . $line;
                    fseek($f, $cursor--, SEEK_END);
                    $char = fgetc($f);
                }

                if (strpos($line, 'CellID') !== false) {
                    $lineArray = explode(',', $line);
                    $lineCellid = str_replace('CellID: ', '', $lineArray[0]);
                    $lineCellid = hexdec($lineCellid);
                    $lineLAC = str_replace('LAC: ', '', $lineArray[1]);
                    $lineLAC = hexdec($lineLAC);
                    /*echo $lineArray[2] . "</br>";
                    echo $lineCellid . "</br>";
                    echo $lineLAC . "</br>";*/

                    $apiKey = "UPDATE_TO_YOURS";
                    $gsmURL = "https://www.opencellid.org/cell/get?key=" . $apiKey . "&mcc=UPDATE_TO_YOURS&mnc=UPDATE_TO_YOURS&lac=" . $lineLAC . "&cellid=" . $lineCellid;
                    $contents = file_get_contents($gsmURL);

                    if($contents !== false){
                        //echo $contents;
                        $latTemp = "";
                        $lngTemp = "";
                        preg_match('/lat="(.+?)"/', $contents, $latTemp);
                        preg_match('/lon="(.+?)"/', $contents, $lngTemp);
                        $line = "lat: " . $latTemp[1] . ", lng: " . $lngTemp[1] . ", " . $lineArray[2] . " - Satellite: CellID: " . $lineCellid . " LAC: " . $lineLAC;
                    }
                }

                return $line;
        }

$output = read_last_line("/var/www/results");
$resultsSeparated = explode(", ", $output);

$latLng = strval($resultsSeparated[0] . ", " . $resultsSeparated[1]);
$dateNow = $resultsSeparated[2];

?>

<html>
  <head>
   <title>GPS Tracker</title>
   <style type="text/css">
      #map {
        height: 400px;
        width: 50%;
      }
    </style>
    <script>
      function initMap() {
        const location = { <?php echo $latLng; ?> };
        const map = new google.maps.Map(document.getElementById("map"), {
          zoom: 15,
          center: location,
        });
        const contentString =
                <?php echo "'" . $dateNow . "'"; ?>;
        const infowindow = new google.maps.InfoWindow({
          content: contentString,
        });
        const marker = new google.maps.Marker({
          position: location,
          map: map,
        });
        marker.addListener("click", () => {
        infowindow.open({
          anchor: marker,
          map,
          shouldFocus: false,
        });
       });
      }
    </script>
  </head>

<body>

    <h2>GPS Tracker</h2>
    <div id="map"></div>
    <script
      src="https://maps.googleapis.com/maps/api/js?key=UPDATE_TO_YOURS&callback=initMap&libraries=&v=weekly"
      async></script>
  </body>
</html>
