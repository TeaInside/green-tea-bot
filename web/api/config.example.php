<?php

date_default_timezone_set("UTC");

const BASE_PATH = __DIR__;
const STORAGE_PATH = BASE_PATH."/storage";
const PUBLIC_PATH = BASE_PATH."/public";
const STATIC_CACHE = false;

const DB_HOST = "127.0.0.1";
const DB_PORT = 3306;
const DB_NAME = "";
const DB_USER = "";
const DB_PASS = "";
const APP_KEY = "";

const PDO_PARAM = [
	"mysql:host=".DB_HOST.";port=".DB_PORT.";dbname=".DB_NAME,
	DB_USER,
	DB_PASS,
	[
		\PDO::ATTR_ERRMODE => \PDO::ERRMODE_EXCEPTION
	]
];
