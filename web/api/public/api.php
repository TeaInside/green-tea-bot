<?php
/* SPDX-License-Identifer: GPL-2.0-only */
/*
 * Copyright (C) 2021  Ammar Faizi <ammarfaizi2@gmail.com>
 *
 * Green Tea Bot API endpoint.
 */

use GreenTea\API\GetGroupList;
use GreenTea\API\GetChatMessages;
use GreenTea\API\RegisterAccount;

if (isset($_SERVER["HTTP_ORIGIN"]) && is_string($_SERVER["HTTP_ORIGIN"])) {
	header("Access-Control-Allow-Origin: {$_SERVER["HTTP_ORIGIN"]}");
} else {
	header("Access-Control-Allow-Origin: *");
}

$msg  = NULL;
$code = 200;

if (!isset($_GET["action"]) || !is_string($_GET["action"])) {
	$msg  = "Missing \"action\" (string) parameter";
	$code = 400;
	goto out;
}

$action = $_GET["action"];

/*
 * Cut off the string at 32 characters to prevent being abused
 * (e.g. generating large response on the "default" case).
 */
$action = substr($action, 0, 32);
unset($_GET["action"]);

try {
	require __DIR__."/../src/autoload.php";
	switch ($action) {
	case "get_group_list":
		$api = new GetGroupList();
		$arg = [];

		if (isset($_GET["limit"]) && is_numeric($_GET["limit"]))
			$arg[0] = (int) $_GET["limit"];

		if (isset($_GET["offset"]) && is_numeric($_GET["offset"])) {
			if (!isset($arg[0]))
				$arg[0] = 100;
			$arg[1] = (int) $_GET["offset"];
		}

		$msg = $api->get(...$arg);
		if ($api->isError())
			$code = $api->getErrorCode();
		break;
	case "get_chat_messages":
		if (!isset($_GET["group_id"]) || !is_string($_GET["group_id"])) {
			$msg  = "Missing \"group_id\" parameter";
			$code = 400;
			goto out;
		}

		$api = new GetChatMessages();
		$arg = [$_GET["group_id"]];

		if (isset($_GET["limit"]) && is_numeric($_GET["limit"]))
			$arg[1] = (int) $_GET["limit"];

		if (isset($_GET["offset"]) && is_numeric($_GET["offset"])) {
			if (!isset($arg[1]))
				$arg[1] = 100;
			$arg[2] = (int) $_GET["offset"];
		}

		$msg = $api->get(...$arg);
		if ($api->isError())
			$code = $api->getErrorCode();
		break;
	case "register":
		$api = new RegisterAccount();
		$msg = $api->register();
		if ($api->isError())
			$code = $api->getErrorCode();
		break;
	default:
		$msg  = "Invalid action \"{$action}\"";
		$code = 400;
	}
} catch (\Error $e) {
	$code = 500;
	$msg  = "Internal server error";

	/*
	 * Note: Use $e->__toString() for debugging purpose only.
	 *       Comment this line out in production!
	 */
	$msg  = $e->__toString();
}


out:
http_response_code($code);
header("Content-Type: application/json");
echo json_encode([
	"code" => $code,
	"msg"  => $msg
], JSON_PRETTY_PRINT | JSON_UNESCAPED_SLASHES);
