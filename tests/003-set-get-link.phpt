--TEST--
set/get/list functions and symlink
--SKIPIF--
<?php
if (!extension_loaded("xattr")) die("skip xattr extension not loaded");
if (!xattr_supported(__FILE__)) die("skip xattr not supported here");
if (exec("id -u")!=="0")        die("skip need root privileges");
?>
--FILE--
<?php 
$attr1 = 'php.test1';
$attr2 = 'php.test2';
$attr3 = 'php.test3';
$fic   = 'foo.txt';
$link  = 'foo.lnk';
file_put_contents($fic, $fic);
symlink($fic, $link);

echo "-- Set\n";
var_dump(xattr_set($fic,  $attr1, 'foo', XATTR_TRUSTED));
var_dump(xattr_set($link, $attr2, 'bar', XATTR_TRUSTED));
var_dump(xattr_set($link, $attr3, 'xxx', XATTR_TRUSTED|XATTR_DONTFOLLOW));

echo "-- Get\n";
var_dump(xattr_get($fic,  $attr1, XATTR_TRUSTED));
var_dump(xattr_get($link, $attr2, XATTR_TRUSTED));
var_dump(xattr_get($link, $attr3, XATTR_TRUSTED|XATTR_DONTFOLLOW));

echo "-- List\n";
var_dump(xattr_list($fic,  XATTR_ALL));
var_dump(xattr_list($link, XATTR_ALL));
var_dump(xattr_list($link, XATTR_ALL|XATTR_DONTFOLLOW));
?>
Done
--CLEAN--
<?php
unlink('foo.txt');
unlink('foo.lnk');
?>
--EXPECT--
-- Set
bool(true)
bool(true)
bool(true)
-- Get
string(3) "foo"
string(3) "bar"
string(3) "xxx"
-- List
array(3) {
  [0]=>
  string(17) "trusted.php.test1"
  [1]=>
  string(17) "trusted.php.test2"
  [2]=>
  string(16) "security.selinux"
}
array(3) {
  [0]=>
  string(17) "trusted.php.test1"
  [1]=>
  string(17) "trusted.php.test2"
  [2]=>
  string(16) "security.selinux"
}
array(2) {
  [0]=>
  string(17) "trusted.php.test3"
  [1]=>
  string(16) "security.selinux"
}
Done
