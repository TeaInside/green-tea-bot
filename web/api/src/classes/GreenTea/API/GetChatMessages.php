<?php
/* SPDX-License-Identifer: GPL-2.0-only */
/*
 * Copyright (C) 2021  Ammar Faizi <ammarfaizi2@gmail.com>
 */

namespace GreenTea\API;

use PDO;
use GreenTea\APIFoundation;

class GetChatMessages extends APIFoundation
{
	const GROUP_ID_EXCEPTIONS = [
		-1001278544502 => true,
		-1001226735471 => true,
	];

	/**
	 * @param int $groupId
	 * @param int $limit
	 * @param int $offset
	 * @return array
	 */
	public function get(string $groupId, int $limit = 100, int $offset = 0): array
	{
		$pdo  = $this->getPDO();
		$isOk = false;
		$msg  = NULL;
		$data = NULL;
		$this->errorCode = 400;

		if (isset(self::GROUP_ID_EXCEPTIONS[$groupId]))
			$groupId = -1;

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

		$query = <<<SQL
		SELECT * FROM (
			SELECT
				gt_messages.id,
				gt_users.tg_user_id,
				gt_users.first_name,
				gt_users.last_name,
				gt_users.username,
				gt_messages.tg_msg_id,
				gt_messages.reply_to_tg_msg_id,
				gt_message_content.text,
				gt_messages.msg_type,
				gt_messages.has_edited_msg,
				gt_messages.is_forwarded_msg,
				gt_messages.is_deleted,
				gt_message_content.tg_date
			FROM gt_messages
			INNER JOIN gt_message_content ON gt_messages.id = gt_message_content.message_id
			INNER JOIN gt_senders ON gt_senders.id = gt_messages.sender_id
			INNER JOIN gt_sender_user ON gt_senders.id = gt_sender_user.sender_id
			INNER JOIN gt_users ON gt_users.id = gt_sender_user.user_id
			INNER JOIN gt_chats ON gt_chats.id = gt_messages.chat_id
			WHERE gt_chats.id = (
				SELECT gt_chat_group.chat_id FROM gt_chat_group
				INNER JOIN gt_groups ON gt_groups.id = gt_chat_group.group_id
				WHERE gt_groups.tg_group_id = ? LIMIT 1
			)
			ORDER BY gt_message_content.tg_date DESC
			LIMIT {$limit} OFFSET {$offset}
		) tmp ORDER BY tg_date ASC;
SQL;
		$st    = $pdo->prepare($query);
		$st->execute([$groupId]);
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
