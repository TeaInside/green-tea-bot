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
		$isOk = true;
		$msg  = NULL;
		$data = [];

		$query = "SELECT * FROM gt_groups LIMIT {$limit} OFFSET {$offset}";
		$st    = $pdo->prepare($query);
		$st->execute();
		$data  = $st->fetchAll(PDO::FETCH_ASSOC);

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
