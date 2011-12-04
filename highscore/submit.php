<?php
/*
 * Black Belt Sorvi Hero
 *
 * Copyright 2011 Janne Kulmala <janne.t.kulmala@iki.fi>,
 * Antti Rajamäki <amikaze@gmail.com>
 *
 * Program code and resources are licensed with GNU LGPL 2.1. See
 * COPYING.LGPL file.
 */

function error($msg) {
	header("HTTP/1.0 500 $msg");
	die("<h1>ERROR: $msg</h1>");
}

$MAX_SCORES = 100;
$HIGHSCORE_SECRET_KEY = 'XXXXXXXX';

if (empty($_POST['name']) or empty($_POST['score']) or
    empty($_POST['sign'])) {
	error('Parameters missing');
}
$new_name = trim($_POST['name']);
$new_name = str_replace("\n", '', $new_name);
$new_name = str_replace("\r", '', $new_name);
if (empty($new_name) or !is_numeric($_POST['score'])
    or strlen($_POST['name']) >= 100) {
	error('Invalid parameters (hack attempt?)');
}
$sign = sha1("$HIGHSCORE_SECRET_KEY $_POST[score] $_POST[name]");
if ($sign != $_POST['sign']) {
	error('Hack attempt');
}
$new_score = intval($_POST['score']);

$added = false;

$f = @fopen('highscores.txt', 'r+');
if ($f === NULL) {
	error('Unable to open file');
}
@flock($f, LOCK_EX);
$table = array();
while (count($table) < $MAX_SCORES) {
	$l = trim(fgets($f));
	if (empty($l)) break;
	$elems = explode(' ', $l, 2);
	if (!is_numeric($elems[0]) or empty($elems[1])) {
		flock($f, LOCK_UN);
		error('Database error');
	}
	$score = intval($elems[0]);
	$name = $elems[1];
	if ($new_score == $score && $new_name == $name) {
		flock($f, LOCK_UN);
		error('Duplicate entry');
	}
	if ($new_score > $score and $added == false) {
		$table[] = array(
			"name" => $new_name,
			"score" => $new_score,
		);
		$added = true;
		if (count($table) >= $MAX_SCORES) break;
	}
	$table[] = array(
		"name" => $name,
		"score" => $score,
	);
}

if ($added == false and count($table) < $MAX_SCORES) {
	$table[] = array(
		"name" => $new_name,
		"score" => $new_score,
	);
	$added = true;
}
if ($added == false) {
	flock($f, LOCK_UN);
	error("Not added to highscore");
}

fseek($f, 0);
$pos = 0;
foreach ($table as $i) {
	$pos += fwrite($f, "$i[score] $i[name]\n");
}
ftruncate($f, $pos);
flock($f, LOCK_UN);
fclose($f);

echo "OK";

