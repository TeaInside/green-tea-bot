<?php
/* SPDX-License-Identifer: GPL-2.0-only */
/*
 * Copyright (C) 2021  Ammar Faizi <ammarfaizi2@gmail.com>
 */

require __DIR__."/../config.php";

function __api_class_autoloader($className)
{
	$className = str_replace("\\", "/", $className);
	$file = __DIR__."/classes/".$className.".php";
	if (file_exists($file))
		require $file;
}

spl_autoload_register("__api_class_autoloader");
