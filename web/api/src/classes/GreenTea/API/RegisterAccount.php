<?php
/* SPDX-License-Identifer: GPL-2.0-only */
/*
 * Copyright (C) 2021  Ammar Faizi <ammarfaizi2@gmail.com>
 */

namespace GreenTea\API;

use PDO;
use GreenTea\APIFoundation;

class RegisterAccount extends APIFoundation
{
	/**
	 * @return bool
	 */
	private function insertUser(array $d, string $userId): bool
	{
		$q = <<<SQL
		INSERT INTO `telegram_sso`
			(`user_id`, `username`, `email`, `password`, `created_at`)
		VALUES
			(?, ?, ?, ?, ?);
SQL;
		$pdo = $this->getPDO();
		$st = $pdo->prepare($q);
		return $st->execute([
			$userId,
			$d["username"],
			$d["email"],
			password_hash($d["password"], PASSWORD_BCRYPT),
			date("Y-m-d H:i:s")
		]);
	}

	/**
	 * @return bool
	 */
	private function emailExists(string $email): bool
	{
		$q = "SELECT id FROM telegram_sso WHERE email = ?";
		$pdo = $this->getPDO();
		$st = $pdo->prepare($q);
		$st->execute([$email]);
		return !!$st->fetch(\PDO::FETCH_NUM);
	}

	/**
	 * @return bool
	 */
	private function usernameExists(string $username): bool
	{
		$q = "SELECT id FROM telegram_sso WHERE username = ?";
		$pdo = $this->getPDO();
		$st = $pdo->prepare($q);
		$st->execute([$username]);
		return !!$st->fetch(\PDO::FETCH_NUM);
	}

	/**
	 * @return bool
	 */
	private function tgUserIdExists(string $tgUserId, string &$userId): bool
	{
		$q = "SELECT id FROM gt_users WHERE tg_user_id = ?";
		$pdo = $this->getPDO();
		$st = $pdo->prepare($q);
		$st->execute([$tgUserId]);
		$r = $st->fetch(\PDO::FETCH_NUM);
		if ($r) {
			$userId = $r[0];
			return true;
		}
		return false;
	}

	/**
	 * @return array
	 */
	public function register(): array
	{
		$msg = "";
		$userId = "";
		$isOk = false;
		$this->errorCode = 400;
		if (!isset($_POST["username"]) || !is_string($_POST["username"])) {
			$msg = "Missing username string";
			goto out;
		}

		if (!isset($_POST["tg_user_id"]) || !is_string($_POST["tg_user_id"])) {
			$msg = "Missing tg_user_id string";
			goto out;
		}

		if (!isset($_POST["email"]) || !is_string($_POST["email"])) {
			$msg = "Missing email string";
			goto out;
		}

		if (!filter_var($_POST["email"], FILTER_VALIDATE_EMAIL)) {
			$msg = "{$_POST["email"]} is not a valid email";
			goto out;
		}

		if (!isset($_POST["password"]) || !is_string($_POST["password"])) {
			$msg = "Missing password string";
			goto out;
		}

		if (!isset($_POST["cpassword"]) || !is_string($_POST["cpassword"])) {
			$msg = "Missing cpassword string";
			goto out;
		}

		if (strlen($_POST["password"]) < 6) {
			$msg = "password must be at least 6 characters";
			goto out;
		}

		if (!preg_match("/\d/", $_POST["password"]) ||
		    !preg_match("/[a-z]/", $_POST["password"]) ||
		    !preg_match("/[A-Z]/", $_POST["password"])) {
			$msg = "password must consist of numeric char, upper case letter and lower case letter";
			goto out;
		}

		if ($_POST["password"] !== $_POST["cpassword"]) {
			$msg = "password must be the same with cpassword";
			goto out;
		}

		if ($this->emailExists($_POST["email"])) {
			$msg = "Email {$_POST["email"]} has been registered, please use another email";
			goto out;
		}

		if ($this->usernameExists($_POST["username"])) {
			$msg = "Username {$_POST["username"]} has been registered, please use another username";
			goto out;
		}

		if (!$this->tgUserIdExists($_POST["tg_user_id"], $userId)) {
			$msg = "tg_user_id {$_POST["tg_user_id"]} does not exist";
			goto out;
		}

		if ($this->insertUser($_POST, $userId)) {
			$this->errorCode = 0;
			$isOk = true;
			$msg  = "success!";
		}
	out:
		return [
			"is_ok" => $isOk,
			"msg"   => $msg,
			"data"  => [],
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
