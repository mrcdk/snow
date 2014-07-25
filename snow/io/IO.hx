package snow.io;

import haxe.io.Path;
import snow.utils.ByteArray;
import snow.utils.Libs;
import sys.FileSystem;

/** This class is a low level cross platform file access helper, that handles in bundle assets etc.
    If you want a file, use `Assets` instead, unless really required. */
class IOFile {

#if snow_native

        /** The internal native file handle */
    public var handle : Dynamic;

    @:noCompletion public function new( _handle:Dynamic ) {
        handle = _handle;
    } //new

        /** Read a `maxnum` of items of `size` in bytes, into `dest`. Same signature/returns as `fread` */
    public function read( dest:ByteArray, size:Int, maxnum:Int ) {
        return snow_iosrc_file_read(handle, dest, size, maxnum);
    } //read

        /** Write `num` of items of `size` in bytes, from `src` into this file. Same signature/returns as `fwrite` */
    public function write( src:ByteArray, size:Int, num:Int ) {
        return snow_iosrc_file_write(handle, src, size, num);
    } //write

        /** Seek `offset` from `whence`, where whence is an int matching `set=0`, `cur=1`, `end=2`. Same signature/returns as `fseek` */
    public function seek( offset:Int, whence:Int ) {
        return snow_iosrc_file_seek(handle, offset, whence);
    } //seek

        /** Tell the current position in the file, in bytes */
    public function tell() {
        return snow_iosrc_file_tell(handle);
    } //tell

        /** Close the file */
    public function close() {
        return snow_iosrc_file_close(handle);
    } //close

    static var snow_iosrc_file_read    = Libs.load( "snow", "snow_iosrc_file_read", 4 );
    static var snow_iosrc_file_write   = Libs.load( "snow", "snow_iosrc_file_write", 4 );
    static var snow_iosrc_file_seek    = Libs.load( "snow", "snow_iosrc_file_seek", 3 );
    static var snow_iosrc_file_tell    = Libs.load( "snow", "snow_iosrc_file_tell", 1 );
    static var snow_iosrc_file_close   = Libs.load( "snow", "snow_iosrc_file_close", 1 );

#end //snow_native

} //IOFile


/** This class is a low level cross platform IO helper.
    If you want file access, use `Assets` instead, unless really required. */
class IO {

#if snow_native

    static var snow_iosrc_from_file = Libs.load( "snow", "snow_iosrc_from_file", 2 );

        /** Create an `IOFile` from a file path `_id`, this bypasses the `Asset` system path helpers. */
    public static function from_file( _id:String, ?_mode:String="rb" ) : IOFile {
        var handle = snow_iosrc_from_file(_id, _mode);
        return new IOFile(handle);
    }

    static function isAbsolute ( path : String ) : Bool {
        if (StringTools.startsWith(path, '/')) return true;
        if (path.charAt(2) == ':') return true;
        return false;
    }

        //convert a path to absolute (if needed) and normalize, remove slash, etc
    static function resolve( _path:String ) {

        if(!isAbsolute(_path)) {
            _path = FileSystem.fullPath(_path);
        }

        _path = Path.normalize(_path);
        _path = Path.removeTrailingSlashes(_path);

        return _path;

    } //resolve

        /** Add a folder to a watch list for notifications when the contents of the folder change */
    public static function watch_add( _path:String ) {
        if(_path != null && _path.length > 0) {
            snow_io_add_watch( resolve(_path) );
        }
    } //watch_add

        /** Remove a folder from the watch list */
    public static function watch_remove( _path:String ) {
        if(_path != null && _path.length > 0) {
            snow_io_remove_watch( resolve(_path) );
        }
    } //watch_remove

    static var snow_io_add_watch        = Libs.load( "snow", "snow_io_add_watch", 1 );
    static var snow_io_remove_watch     = Libs.load( "snow", "snow_io_remove_watch", 1 );

#end //snow_native

} //IO