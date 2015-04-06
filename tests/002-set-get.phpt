--TEST--
set/get/list/remove functions
--SKIPIF--
<?php
if (!extension_loaded("xattr")) die("skip xattr extension not loaded");
if (!xattr_supported(".")       die("skip xattr not supported here");
?>
--FILE--
<?php 
$attr = 'php.test';
$fic  = 'foo.txt';
file_put_contents($fic, $fic);
var_dump(xattr_supported($fic));

var_dump(xattr_set($fic, $attr, 'foo'));
var_dump(xattr_get($fic, $attr));

var_dump(xattr_set($fic, $attr, 'bar'));
var_dump(xattr_get($fic, $attr));

var_dump(xattr_list($fic));

var_dump(xattr_remove($fic, $attr));
var_dump(xattr_get($fic, $attr));
?>
Done
--CLEAN--
<?php unlink('foo.txt'); ?>
--EXPECT--
bool(true)
bool(true)
string(3) "foo"
bool(true)
string(3) "bar"
array(1) {
  [0]=>
  string(8) "php.test"
}
bool(true)
bool(false)
Done
