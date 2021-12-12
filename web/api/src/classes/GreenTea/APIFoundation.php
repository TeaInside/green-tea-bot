<?php
/* SPDX-License-Identifer: GPL-2.0-only */
/*
 * Copyright (C) 2021  Ammar Faizi <ammarfaizi2@gmail.com>
 */

namespace GreenTea;

use PDO;

abstract class APIFoundation
{
	/**
	 * @var ?\PDO
	 */
	private ?PDO $pdo = NULL;

	/**
	 * @var int
	 */
	protected int $errorCode = 0;

	/**
	 * @return \PDO
	 */
	protected function createPDO(?array $PDOParam = NULL): PDO
	{
		if ($PDOParam === NULL)
			$PDOParam = PDO_PARAM;

		return new \PDO(...$PDOParam);
	}

	/**
	 * @return \PDO
	 */
	protected function getPDO(): PDO
	{
		if (!$this->pdo)
			$this->pdo = $this->createPDO();

		return $this->pdo;
	}
};
