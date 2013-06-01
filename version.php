<?php
	define("CURRENT_VERSION", 0x00060800); //0.6.8.0
	if ($_GET['ver'] >= CURRENT_VERSION) {
		echo '^';
		return;
	}
	$strs = explode(" ", $_GET['os']);
	$filename = "phyros-";
	if (($strs[0] == "Microsoft") &&
		($strs[1] == "Windows")) {
		$filename .= "win";
		switch ($strs[4]) {
			case "x86":
			case "IA32":
				$filename .= "-x86.upd";
				break;
			case "AMD64":
				$filename .= "-amd64.upd";
				break;
			default:
				echo ",";
				return;
		}
	} else {
		if (ereg("86$", $strs[count($strs) - 1]))
			$filename .= strtolower($strs[0]) . "-x86.upd";
		else
			echo ',';
	}
	$filename = "/phyros/updates/" . $filename;
	if (file_exists($_SERVER{'DOCUMENT_ROOT'} . $filename))
		echo CURRENT_VERSION . ' ' . $filename . ' ' . sha1_file($_SERVER{'DOCUMENT_ROOT'} . $filename);
	else
		echo ",";
?>
