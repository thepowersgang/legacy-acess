<?php
//$regex1 = "/^\s*[\.a-zA-Z]+\s+0x([0-9a-f]{8})\s+0x[0-9a-f]+\s+([_a-zA-Z0-9\.]+)$/";
$regex2 = "/^\s*0x([0-9a-f]{8})\s+([_a-zA-Z0-9\.]+)$/";

$data = file("map.txt");

foreach($data as $line) {
	$line = trim($line);
	//if(preg_match($regex1, $line, $matches) == 0) {
		if(preg_match($regex2, $line, $matches) == 0)
			continue;
	//}
	//print_r($matches);
	echo $matches[1]," ", $matches[2],"\r\n";
}
?>