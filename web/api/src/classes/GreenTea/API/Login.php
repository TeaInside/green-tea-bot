<?php
/* SPDX-License-Identifer: GPL-2.0-only */
/*
 * Copyright (C) 2021  Ammar Faizi <ammarfaizi2@gmail.com>
 */

namespace GreenTea\API;

use PDO;
use GreenTea\APIFoundation;

function genRandStr(int $n = 32): string
{
	$r = "";
	$list = "qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM1234567890";
	$c = strlen($list);

	for ($i = 0; $i < $n; $i++)
		$r .= $list[rand(0, $c - 1)];

	return $r;
}

class Login extends APIFoundation
{
	/**
	 * @param int $userId
	 */
	private function createSession(PDO $pdo, int $userId): ?string
	{
		$token = genRandStr(64);
		$query = "INSERT INTO `login_sessions` (`user_id`, `token`, `created_at`, `expired_at`) VALUES (?, ?, ?, ?);";
		$st = $pdo->prepare($query);
		$st->execute([
			$userId,
			$token,
			date("Y-m-d H:i:s"),
			date("Y-m-d H:i:s", time() + (3600 * 24 * 2))
		]);
		return $token;
	}

	/**
	 * @param string $email
	 * @param string $pass
	 */
	public function doLogin(string $email, string $pass): array
	{
		$pdo  = $this->getPDO();
		$isOk = false;
		$msg  = NULL;
		$data = NULL;
		$this->errorCode = 400;

		$query = "SELECT id, user_id, password FROM telegram_sso WHERE email = ?";
		$st = $pdo->prepare($query);
		$st->execute([$email]);
		$r = $st->fetch(PDO::FETCH_ASSOC);

		if (isset($r["password"]) && password_verify($pass, $r["password"])) {
			$isOk  = true;
			$token = $this->createSession($pdo, (int) $r["id"]);
			$msg   = ["token" => $token];
		} else {
			$isOk  = false;
			$msg   = ["msg" => "Login failed!"];
		}

		return [
			"is_ok" => $isOk,
			"msg"   => $msg,
			"data"  => $data,
		];
	}

	/**
	 * @return int
	 */
	public function getErrorCode(): int
	{
		return $this->errorCode;
	}

	/**
	 * @return bool
	 */
	public function isError(): bool
	{
		return $this->errorCode != 0;
	}
};
