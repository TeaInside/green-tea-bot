<?php
/* SPDX-License-Identifer: GPL-2.0-only */
/*
 * Copyright (C) 2021  Ammar Faizi <ammarfaizi2@gmail.com>
 */

namespace GreenTea\API;

use PDO;
use GreenTea\APIFoundation;

class GetGroupList extends APIFoundation
{
	/**
	 * @param int $limit
	 * @param int $offset
	 * @return array
	 */
	public function get(int $limit = 100, int $offset = 0): array
	{
		$pdo  = $this->getPDO();
		$isOk = false;
		$msg  = NULL;
		$data = NULL;
		$this->errorCode = 400;

		if ($limit < 0) {
			$msg  = sprintf("limit cannot be negative (given limit %d)", $limit);
			goto out;
		}
		if ($limit > 300) {
			$msg  = sprintf("limit cannot be greater than 300 (given limit %d)", $limit);
			goto out;
		}
		if ($offset < 0) {
			$msg  = sprintf("offset cannot be negative (given offset %d)", $offset);
			goto out;
		}

		$query = "SELECT * FROM gt_groups ORDER BY id ASC LIMIT {$limit} OFFSET {$offset}";
		$st    = $pdo->prepare($query);
		$st->execute();
		$data  = $st->fetchAll(PDO::FETCH_ASSOC);
		$isOk  = true;
		$this->errorCode = 0;

	out:
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
