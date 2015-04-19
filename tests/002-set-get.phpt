--TEST--
set/get/list/remove functions
--SKIPIF--
<?php
if (!extension_loaded("xattr")) die("skip xattr extension not loaded");
if (!xattr_supported(__FILE__)) die("skip xattr not supported here");
?>
--FILE--
<?php 
$attr = 'php.test';
$fic  = 'foo.txt';
echo "-- Supported\n";
file_put_contents($fic, $fic);
var_dump(xattr_supported($fic));

echo "-- Set/Get\n";
var_dump(xattr_set($fic, $attr, 'foo'));
var_dump(xattr_get($fic, $attr));
var_dump(xattr_get($fic, $attr, XATTR_USER));
var_dump(xattr_get($fic, "user.$attr", XATTR_ALL));

var_dump(xattr_set($fic, $attr, 'bar'));
var_dump(xattr_get($fic, $attr));

echo "-- List\n";
var_dump(xattr_list($fic));
var_dump(in_array("user.$attr", xattr_list($fic, XATTR_ALL)));

echo "-- Remove\n";
var_dump(xattr_remove($fic, $attr));
var_dump(xattr_get($fic, $attr));

?>
Done
--CLEAN--
<?php unlink('foo.txt'); ?>
--EXPECT--
-- Supported
bool(true)
-- Set/Get
bool(true)
string(3) "foo"
string(3) "foo"
string(3) "foo"
bool(true)
string(3) "bar"
-- List
array(1) {
  [0]=>
  string(8) "php.test"
}
bool(true)
-- Remove
bool(true)
bool(false)
Done
