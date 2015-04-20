--TEST--
set function with create/replace options
--SKIPIF--
<?php
if (!extension_loaded("xattr")) die("skip xattr extension not loaded");
if (!xattr_supported(__FILE__)) die("skip xattr not supported here");
?>
--FILE--
<?php 
$attr = 'php.test';
$fic  = 'foo.txt';

file_put_contents($fic, $fic);

var_dump(xattr_set($fic, $attr, '1', XATTR_REPLACE)); /* fails */
var_dump(xattr_get($fic, $attr));
var_dump(xattr_set($fic, $attr, '2', XATTR_CREATE));
var_dump(xattr_get($fic, $attr));
var_dump(xattr_set($fic, $attr, '3', XATTR_CREATE)); /* fails */
var_dump(xattr_get($fic, $attr));
var_dump(xattr_set($fic, $attr, '4', XATTR_REPLACE));
var_dump(xattr_get($fic, $attr));
var_dump(xattr_set($fic, $attr, '5'));
var_dump(xattr_get($fic, $attr));

?>
Done
--CLEAN--
<?php unlink('foo.txt'); ?>
--EXPECTF--

Warning: xattr_set Attribute user.php.test doesn't exists in %s004-create-replace.php on line 7
bool(false)
bool(false)
bool(true)
string(1) "2"

Warning: xattr_set Attribute user.php.test already exists in %s004-create-replace.php on line 11
bool(false)
string(1) "2"
bool(true)
string(1) "4"
bool(true)
string(1) "5"
Done
