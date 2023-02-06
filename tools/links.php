<?php
$FILES = array("disk.c", "exe_elf.c", "fs_devfs.c", "fs_ext2.c", "fs_fat.c", "fs_internal.c",
	"gdt.c", "hdd.c", "idt.c", "irq.c", "isrs.c", "kb.c", "lib.c", "main.c", "mm.c", "proc.c",
	"scrn.c", "syscalls.c", "time.c", "vfs.c");
$defFUNCTIONS = array();
$useFUNCTIONS = array();
define("IMG_W", 800);
define("IMG_H", 800);
define("ARROW_SIZE", 10);

function imagearrow($img, $x, $y, $s, $angle, $color)
{
	$points = array(0,-0.5, -0.5,0.5, 0.5,0.5);
	for($i=0;$i<6;$i+=2) {
		$nx = cos($angle)*$points[$i] - sin($angle)*$points[$i+1];
		$ny = sin($angle)*$points[$i] + cos($angle)*$points[$i+1];
		$points[$i] = $nx;
		$points[$i+1] = $ny;
		
		$points[$i] += 0.5;		$points[$i] *= $s;		$points[$i] += $x;		
		$points[$i+1] += 0.5;	$points[$i+1] *= $s;	$points[$i+1] += $y;
	}
	
	imagefilledpolygon($img, $points, 3, $color);
}

foreach($FILES as $i=>$file)
{
	$data = file_get_contents($file);
	preg_replace("/\/\*.*?\*\//", "", $data);
	preg_replace("/\/\/.*?$/", "", $data);
	echo $file,"\n";
	preg_match_all("/\b[a-zA-Z_][a-zA-Z0-9_]* \*?([a-zA-Z][a-zA-Z0-9_]*)\s*\(/", $data, $matches);
	if($i==3)
		print_r($matches);
	foreach($matches[1] as $match) {
		$defFUNCTIONS[] = trim($match);
		$defFUNCTIONSid[] = $i;
	}
	
	preg_match_all("/([a-zA-Z][a-zA-Z0-9_]*)\(/", $data, $matches);
	//if($i==3)
	//	print_r($matches);
	$useFUNCTIONS[] = array();
	foreach($matches[1] as $match) {
		switch($match) {
		case "if":
		case "switch":
		case "for":
		case "while":
		case "asm__":
		case "sizeof":
			break;
		default:
			$useFUNCTIONS[$i][] = trim($match);
		}
	}
	//print_r($useFUNCTIONS[$i]);
	//echo "\n\n";
}

//exit;
//$defFUNCTIONS = array_unique($defFUNCTIONS);
//$fp=fopen("1.txt","w");
//fwrite($fp,print_r($defFUNCTIONS,1));
//fclose($fp);
//exit;

foreach($useFUNCTIONS as $i=>$fileFs)
{
	//print_r($fileFs);
	foreach($fileFs as $func)
	{
		$id = array_search($func, $defFUNCTIONS);
		if($id === FALSE)
			continue;
		//	die("Undefined Function '$func'\n");
		if($defFUNCTIONSid[$id] == $i)
			continue;
		$LINKS[$i][$id]++;
	}
}

$im = imagecreatetruecolor(IMG_W, IMG_H);
$white = imagecolorallocate($im, 255, 255, 255);
$red = imagecolorallocate($im, 255, 0, 0);
imagefilledrectangle($im, 0, 0, IMG_W, IMG_H, $white);

$deltaRad = M_PI*2/count($FILES);
foreach($FILES as $i=>$file)
{
	$points[] = array(IMG_W/2+7*IMG_W/16*sin($i*$deltaRad), IMG_H/2-7*IMG_W/16*cos($i*$deltaRad));
}

foreach($LINKS as $i=>$file)
{
	//sort($file);
	$j = $i * 0x20;
	foreach($file as $func=>$times) {
		$b = $j&0xFF;
		$g = ($j&0xFF00)>>8;
		$r = ($j&0xFF0000)>>16;
		
		$sx = $points[$i][0];	$sy = $points[$i][1];
		$ex = $points[$defFUNCTIONSid[$func]][0];
		$ey = $points[$defFUNCTIONSid[$func]][1];
		$col = imagecolorallocate($im, $r, $g, $b);
		
		imageline($im, $sx, $sy,  $ex, $ey,  $col);
		imagearrow($im, $sx + ($ex-$sx)*0.75, $sy + ($ey-$sy)*0.75 - ARROW_SIZE/2, ARROW_SIZE, atan(($ey-$sy)/($ex-$sx))+M_PI/2, $col);
	}
	imagestring($im, 3, $points[$i][0], $points[$i][1], $FILES[$i], $red);
}
imagepng($im, "links.png");
imagedestroy($im);
?>