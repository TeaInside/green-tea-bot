<?php
/* SPDX-License-Identifer: GPL-2.0-only */
/*
 * Copyright (C) 2021  Ammar Faizi <ammarfaizi2@gmail.com>
 */

namespace GreenTea\API;

use PDO;
use GreenTea\APIFoundation;

class GetMessageCountGroup extends APIFoundation
{
	/**
	 * @return array
	 */
	public function getCount(): array
	{
		$pdo = $this->getPDO();
		$date = date("Y-m-d");
		$query = <<<SQL
			SELECT
			gt_groups.name, COUNT(1) AS msg_count
			FROM gt_messages
			INNER JOIN gt_message_content
			ON gt_messages.id = gt_message_content.id
			INNER JOIN gt_groups
			ON gt_groups.id = gt_messages.chat_id WHERE
			gt_message_content.tg_date >= '{$date} 00:00:00' AND
			gt_message_content.tg_date <= '{$date} 23:59:59'
			GROUP BY gt_messages.chat_id
			ORDER BY msg_count DESC;
SQL;
		$st = $pdo->prepare($query);
		$st->execute();
		$this->errorCode = 0;
		return [
			"is_ok" => true,
			"msg"   => NULL,
			"data"  => $st->fetchAll(PDO::FETCH_ASSOC),
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
