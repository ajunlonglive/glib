<?php
use Glib\Source;

if(PHP_VERSION_ID < 701000) {
    include __DIR__ . '/source_test_old.inc';
    die;
}

class test extends Source {
    public function prepare() : array { return [true, 0];}
    public function check() : bool { return true;}
    public function dispatch(?callable $callback, ?array $args) : bool { return true;}
    public function finalize() : void { }
}