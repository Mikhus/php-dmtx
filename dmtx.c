/*
   +----------------------------------------------------------------------+
   | PHP Version 5 / dmtx                                                 |
   +----------------------------------------------------------------------+
   | Copyright (c) 2006-2007 Mikko Koppanen                               |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author: Mikko Kopppanen <mkoppanen@php.net>                          |
   +----------------------------------------------------------------------+
*/

#include "php_dmtx.h"

/* Handlers */
static zend_object_handlers php_dmtx_object_handlers;
static zend_object_handlers php_dmtx_read_object_handlers;
static zend_object_handlers php_dmtx_write_object_handlers;

/* Class entries */
zend_class_entry *php_dmtx_sc_entry;
zend_class_entry *php_dmtx_read_sc_entry;
zend_class_entry *php_dmtx_write_sc_entry;
zend_class_entry *php_dmtx_exception_class_entry;

#define PHP_DMTX_MATRIX 1
#define PHP_DMTX_MOSAIC 2

#define X_RESOLUTION 72
#define Y_RESOLUTION 72

#define PHP_DMTX_THROW_IMAGE_EXCEPTION(magick_wand, alternative_message) \
{ \
	ExceptionType severity; \
	char *message = MagickGetException(magick_wand, &severity); \
	MagickClearException(magick_wand); \
	if (message != NULL && strlen(message)) { \
		zend_throw_exception(php_dmtx_exception_class_entry, message, 1 TSRMLS_CC); \
		message = (char *)MagickRelinquishMemory(message); \
		message = (char *)NULL; \
		return; \
	} else { \
		zend_throw_exception(php_dmtx_exception_class_entry, alternative_message, 1 TSRMLS_CC); \
		return; \
	} \
} \

#define PHP_DMTX_THROW_GENERIC_EXCEPTION(message) \
	zend_throw_exception(php_dmtx_exception_class_entry, message, 1 TSRMLS_CC); \
	return; \

#define PHP_DMTX_REGISTER_CONST_LONG(const_name, value) \
	zend_declare_class_constant_long(php_dmtx_sc_entry, const_name, sizeof(const_name)-1, (long)value TSRMLS_CC); \

#define PHP_DMTX_THROW_RW_ERROR_EXCEPTION(filename, code) \
	switch(code) { \
		case PHP_DMTX_NO_FILENAME_ERROR: \
			zend_throw_exception(php_dmtx_exception_class_entry, "No filename specified", 1 TSRMLS_CC); \
			return; \
		break; \
		case PHP_DMTX_ALLOW_URL_FOPEN_ERROR: \
 			zend_throw_exception(php_dmtx_exception_class_entry, "Tried to open from an url but allow_url_fopen is off", 1 TSRMLS_CC); \
			return; \
		break; \
		case PHP_DMTX_UNABLE_TO_READ_FILENAME_ERROR: \
			zend_throw_exception(php_dmtx_exception_class_entry, "Unknown error while reading the filename", 1 TSRMLS_CC); \
			return; \
		break; \
		case PHP_DMTX_FILENAME_TOO_LONG_ERROR: \
			zend_throw_exception(php_dmtx_exception_class_entry, "Filename length exceeds the maximum allowed size", 1 TSRMLS_CC); \
			return; \
		break; \
		case PHP_DMTX_SAFE_MODE_ERROR: \
			zend_throw_exception_ex(php_dmtx_exception_class_entry, 1 TSRMLS_CC, "Safe mode restriction in effect. User is not allowed to access file(%s)", filename); \
			return; \
		break; \
		case PHP_DMTX_OPEN_BASEDIR_ERROR: \
			zend_throw_exception_ex(php_dmtx_exception_class_entry, 1 TSRMLS_CC, "Open basedir restriction in effect. File(%s) is not within allowed path(s)", filename); \
			return; \
		break; \
	} \

ZEND_BEGIN_ARG_INFO_EX(dmtxread_empty_args, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(dmtxread_construct_args, 0, 0, 0)
	ZEND_ARG_INFO(0, filename)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(dmtxread_loadfile_args, 0, 0, 1)
	ZEND_ARG_INFO(0, filename)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(dmtxread_loadstring_args, 0, 0, 1)
	ZEND_ARG_INFO(0, image_string)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(dmtxread_settimeout_args, 0, 0, 1)
	ZEND_ARG_INFO(0, timeout)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(dmtxread_setshrink_args, 0, 0, 1)
	ZEND_ARG_INFO(0, shrink)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(dmtxread_setsymbolshape_args, 0, 0, 1)
	ZEND_ARG_INFO(0, shape)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(dmtxread_setlimit_args, 0, 0, 1)
	ZEND_ARG_INFO(0, start)
	ZEND_ARG_INFO(0, limit)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(dmtxread_setscheme_args, 0, 0, 1)
	ZEND_ARG_INFO(0, scheme)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(dmtxread_getinfo_args, 0, 0, 0)
	ZEND_ARG_INFO(0, scan_gap)
	ZEND_ARG_INFO(0, corrections)
	ZEND_ARG_INFO(0, type)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(dmtxread_setscanregion_args, 0, 0, 4)
	ZEND_ARG_INFO(0, x_min)
	ZEND_ARG_INFO(0, x_max)
	ZEND_ARG_INFO(0, y_min)
	ZEND_ARG_INFO(0, y_max)
ZEND_END_ARG_INFO()

#if PHP_VERSION_ID < 50399
static function_entry php_dmtx_read_class_methods[] =
#else
static zend_function_entry php_dmtx_read_class_methods[] =
#endif
{
	PHP_ME(dmtxread, __construct, dmtxread_construct_args, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(dmtxread, loadfile, dmtxread_loadfile_args, ZEND_ACC_PUBLIC)
	PHP_ME(dmtxread, loadstring, dmtxread_loadstring_args, ZEND_ACC_PUBLIC)
	PHP_ME(dmtxread, settimeout, dmtxread_settimeout_args, ZEND_ACC_PUBLIC)
	PHP_ME(dmtxread, setlimit, dmtxread_setlimit_args, ZEND_ACC_PUBLIC)
	PHP_ME(dmtxread, setsymbolshape, dmtxread_setsymbolshape_args, ZEND_ACC_PUBLIC)
	PHP_ME(dmtxread, setshrink, dmtxread_setshrink_args, ZEND_ACC_PUBLIC)
	PHP_ME(dmtxread, setscanregion, dmtxread_setscanregion_args, ZEND_ACC_PUBLIC)
	PHP_ME(dmtxread, setscheme, dmtxread_setscheme_args, ZEND_ACC_PUBLIC)
	
	PHP_ME(dmtxread, getinfo, dmtxread_getinfo_args, ZEND_ACC_PUBLIC)

	
	PHP_ME(dmtxread, gettimeout, dmtxread_empty_args, ZEND_ACC_PUBLIC)
	PHP_ME(dmtxread, getlimit, dmtxread_empty_args, ZEND_ACC_PUBLIC)
	PHP_ME(dmtxread, getsymbolshape, dmtxread_empty_args, ZEND_ACC_PUBLIC)
	PHP_ME(dmtxread, getshrink, dmtxread_empty_args, ZEND_ACC_PUBLIC)
	PHP_ME(dmtxread, getscanregion, dmtxread_empty_args, ZEND_ACC_PUBLIC)
	PHP_ME(dmtxread, getscheme, dmtxread_empty_args, ZEND_ACC_PUBLIC)
	PHP_ME(dmtxread, unsetscanregion, dmtxread_empty_args, ZEND_ACC_PUBLIC)
	{ NULL, NULL, NULL }
};

ZEND_BEGIN_ARG_INFO_EX(dmtxwrite_empty_args, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(dmtxwrite_construct_args, 0, 0, 0)
	ZEND_ARG_INFO(0, message)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO_EX(dmtxwrite_setmessage_args, 0, 0, 1)
	ZEND_ARG_INFO(0, message)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(dmtxwrite_setscheme_args, 0, 0, 1)
	ZEND_ARG_INFO(0, scheme)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(dmtxwrite_save_args, 0, 0, 1)
	ZEND_ARG_INFO(0, filename)
	ZEND_ARG_INFO(0, symbol)
	ZEND_ARG_INFO(0, type)
ZEND_END_ARG_INFO()

#if PHP_VERSION_ID < 50399
static function_entry php_dmtx_write_class_methods[] =
#else
static zend_function_entry php_dmtx_write_class_methods[] =
#endif
{
	PHP_ME(dmtxwrite, __construct, dmtxwrite_construct_args, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(dmtxwrite, setmessage, dmtxwrite_setmessage_args, ZEND_ACC_PUBLIC)
	PHP_ME(dmtxwrite, setscheme, dmtxwrite_setscheme_args, ZEND_ACC_PUBLIC) // added
	PHP_ME(dmtxwrite, getscheme, dmtxwrite_empty_args, ZEND_ACC_PUBLIC)
	PHP_ME(dmtxwrite, save, dmtxwrite_save_args, ZEND_ACC_PUBLIC)
	{ NULL, NULL, NULL }
};

#if PHP_VERSION_ID < 50399
static function_entry php_dmtx_class_methods[] =
#else
static zend_function_entry php_dmtx_class_methods[] =
#endif
{
	{ NULL, NULL, NULL }
};

/* Create DmtxImage from MagickWand */
DmtxImage *php_create_dmtx_image_from_wand(MagickWand *magick_wand TSRMLS_DC)
{
	long width, height;
	unsigned char *pixels;

	/* Make sure that image is RGB */
	MagickSetImageColorspace(magick_wand, RGBColorspace);

	width  = MagickGetImageWidth(magick_wand);
	height = MagickGetImageHeight(magick_wand);

	pixels = emalloc((3 * width * height * sizeof(unsigned char)));

	if (!MagickExportImagePixels(magick_wand, 0, 0, width, height, "RGB", CharPixel, pixels)) {
		efree(pixels);
		return NULL;
	}

	return dmtxImageCreate(pixels, width, height, DmtxPack24bppRGB);
}

/* {{{ proto void dmtxRead::__construct([string filename])
	Constructs a new dmtxRead object */
PHP_METHOD(dmtxread, __construct)
{
	php_dmtx_read_object *intern;
	char *filename = NULL;
	int filename_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|s!", &filename, &filename_len) == FAILURE) {
		return;
	}

	intern = (php_dmtx_read_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
	intern->options.scheme = PhpDmtxSchemeBase256; //added

	if (filename && filename_len > 0) {
		MagickSetResolution(intern->magick_wand, X_RESOLUTION, Y_RESOLUTION);
		if (MagickReadImage(intern->magick_wand, filename) == MagickFalse) {
			PHP_DMTX_THROW_IMAGE_EXCEPTION(intern->magick_wand, "Unable to read the image");
		}
	}
	return;
}
/* }}} */

/* {{{ proto bool dmtxRead::loadFile(string filename)
	Loads a file into the object */
PHP_METHOD(dmtxread, loadfile)
{
	php_dmtx_read_object *intern;
	char *filename;
	int filename_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &filename, &filename_len) == FAILURE) {
		return;
	}

	intern = (php_dmtx_read_object *)zend_object_store_get_object(getThis() TSRMLS_CC);

	MagickSetResolution(intern->magick_wand, X_RESOLUTION, Y_RESOLUTION);
	if (MagickReadImage(intern->magick_wand, filename) == MagickFalse) {
		PHP_DMTX_THROW_IMAGE_EXCEPTION(intern->magick_wand, "Unable to read the image");
	}
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool dmtxRead::loadString(string filename)
	Loads a string into the object */
PHP_METHOD(dmtxread, loadstring)
{
	php_dmtx_read_object *intern;
	char *image_string;
	int image_string_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &image_string, &image_string_len) == FAILURE) {
		return;
	}

	intern = (php_dmtx_read_object *)zend_object_store_get_object(getThis() TSRMLS_CC);

	MagickSetResolution(intern->magick_wand, X_RESOLUTION, Y_RESOLUTION);
	if (MagickReadImageBlob(intern->magick_wand, image_string, image_string_len) == MagickFalse) {
		PHP_DMTX_THROW_IMAGE_EXCEPTION(intern->magick_wand, "Unable to read the image");
	}

	RETURN_TRUE;
}
/* }}} */

PHP_METHOD(dmtxread, gettimeout)
{
	php_dmtx_read_object *intern;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "") == FAILURE) {
		return;
	}
	
	intern = (php_dmtx_read_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
	RETURN_LONG(intern->options.timeout_ms);
}

PHP_METHOD(dmtxread, getlimit)
{
	php_dmtx_read_object *intern;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "") == FAILURE) {
		return;
	}	
	
	intern = (php_dmtx_read_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
	array_init(return_value);
	add_assoc_long(return_value, "start", intern->options.start);
	add_assoc_long(return_value, "limit", intern->options.limit);
	return;
}

PHP_METHOD(dmtxread, getsymbolshape)
{
	php_dmtx_read_object *intern;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "") == FAILURE) {
		return;
	}
	intern = (php_dmtx_read_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
	RETURN_LONG(intern->options.symbol);
}

PHP_METHOD(dmtxread, getshrink)
{
	php_dmtx_read_object *intern;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "") == FAILURE) {
		return;
	}
	
	intern = (php_dmtx_read_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
	RETURN_LONG(intern->options.shrink);
}


/* {{{ proto bool dmtxRead::setTimeout(int timeout)
	Set timeout for reading. Negative number unsets timeout */
PHP_METHOD(dmtxread, settimeout)
{
	php_dmtx_read_object *intern;
	long timeout_ms;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &timeout_ms) == FAILURE) {
		return;
	}

	intern = (php_dmtx_read_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
	intern->options.timeout_ms = timeout_ms;
	
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool dmtxRead::setLimit(int start, int limit)
	Limit which pages to scan */
PHP_METHOD(dmtxread, setlimit)
{
	php_dmtx_read_object *intern;
	long start, limit = -1;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l|l", &start, &limit) == FAILURE) {
		return;
	}

	intern = (php_dmtx_read_object *)zend_object_store_get_object(getThis() TSRMLS_CC);

	intern->options.start = (start < 0) ? 0 : start;
	intern->options.limit = limit;
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool dmtxRead::setSymbolShape(int shape)
	Symbol shape */
PHP_METHOD(dmtxread, setsymbolshape)
{
	php_dmtx_read_object *intern;
	long symbol;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &symbol) == FAILURE) {
		return;
	}

	intern = (php_dmtx_read_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
	
	if (symbol == DmtxSymbolSquareAuto || symbol == DmtxSymbolSquareCount || 
		symbol == DmtxSymbolRectAuto || symbol == DmtxSymbolRectCount || symbol == DmtxSymbolShapeAuto) {
		intern->options.symbol = symbol;
	} else {
		PHP_DMTX_THROW_GENERIC_EXCEPTION("Unknown symbol shape");
	}
	
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool dmtxRead::setShrink(int shrink)
	set shrink */
PHP_METHOD(dmtxread, setshrink)
{
	php_dmtx_read_object *intern;
	long shrink;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &shrink) == FAILURE) {
		return;
	}

	intern = (php_dmtx_read_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
	intern->options.shrink = shrink;
	
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool dmtxRead::setScanRegion(int x_min, int x_max, int y_min, int y_max)
	Set scan region */
PHP_METHOD(dmtxread, setscanregion)
{
	php_dmtx_read_object *intern;
	long x_min, x_max, y_min, y_max;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "llll", &x_min, &x_max, &y_min, &y_max) == FAILURE) {
		return;
	}

	intern = (php_dmtx_read_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
	
	intern->scan_region.x_min = (x_min < 0) ? 0 : x_min;
	intern->scan_region.x_max = x_max;
	
	intern->scan_region.y_min = (y_min < 0) ? 0 : y_min;
	intern->scan_region.y_max = y_max;
	
	intern->scan_region.active = 1;
	
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool dmtxRead::setScheme([integer scheme])
	Sets the encoding scheme on the object
*/
PHP_METHOD(dmtxread, setscheme)
{
 	php_dmtx_read_object *intern;
	long scheme;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &scheme) == FAILURE) {
		return;
	}

	if (scheme < PhpDmtxSchemeAscii || scheme > PhpDmtxSchemeBase256) {
		PHP_DMTX_THROW_GENERIC_EXCEPTION("Invalid encoding scheme assigned!");
	}

	intern = (php_dmtx_read_object *)zend_object_store_get_object(getThis() TSRMLS_CC);

	intern->options.scheme = scheme;

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool dmtxRead::getScheme()
	Returns encoding scheme on the object
*/
PHP_METHOD(dmtxread, getscheme)
{
	php_dmtx_read_object *intern;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "") == FAILURE) {
		return;
	}

	intern = (php_dmtx_read_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
	RETURN_LONG(intern->options.scheme);
}
/* }}} */

/* {{{ proto bool dmtxRead::unsetScanRegion()
	Unset scan region */
PHP_METHOD(dmtxread, unsetscanregion)
{
	php_dmtx_read_object *intern;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "") == FAILURE) {
		return;
	}

	intern = (php_dmtx_read_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
	intern->scan_region.active = 0;
	
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto array dmtxRead::getScanRegion()
	get scan region */
PHP_METHOD(dmtxread, getscanregion)
{
	php_dmtx_read_object *intern;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "") == FAILURE) {
		return;
	}

	intern = (php_dmtx_read_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
	if (!intern->scan_region.active) {
		return;
	}	
	
	array_init(return_value);	
	add_assoc_long(return_value, "x_min", intern->scan_region.x_min);
	add_assoc_long(return_value, "x_max", intern->scan_region.x_max);
	add_assoc_long(return_value, "y_min", intern->scan_region.y_min);
	add_assoc_long(return_value, "y_max", intern->scan_region.y_max);

	return;
}
/* }}} */

/* {{{ static void _add_assoc_pixel(zval *array, char *key, DmtxPixelLoc pixel) */
static void _add_assoc_pixel(zval *array, char *key, DmtxPixelLoc pixel)
{
	char *buffer;

	spprintf(&buffer, 512, "%dx%d", pixel.X, pixel.Y);
	add_assoc_string(array, key, buffer, 0);
}
/* }}} */

/* {{{ static zval *_php_dmtx_region_to_array(DmtxDecode *decode, DmtxRegion *region, int type, int corrections TSRMLS_DC) */
static zval *_php_dmtx_region_to_array(DmtxDecode *decode, DmtxRegion *region, int type, int corrections TSRMLS_DC)
{
	zval *array, *edges, *bounds;
	DmtxMessage *message;
	
	double rotate;
	int rotate_int;

	/* This is the message */
	if (type == PHP_DMTX_MOSAIC) {
		message = dmtxDecodeMosaicRegion(decode, region, corrections);
	} else {
		message = dmtxDecodeMatrixRegion(decode, region, corrections);
	}

	if (!message) {
		return NULL;
	}
	
	MAKE_STD_ZVAL(array);
	array_init(array);

	add_assoc_string(array, "message", (char *)message->output, 1);
	add_assoc_long(array, "codewords", message->outputIdx);
	
	dmtxMessageDestroy(&message);
	
	rotate = (2 * M_PI) + (atan2(region->fit2raw[0][1], region->fit2raw[1][1]) - atan2(region->fit2raw[1][0], region->fit2raw[0][0])) / 2.0;
	rotate_int = (int)(rotate * 180/M_PI + 0.5);
     
	if (rotate_int >= 360) {
		rotate_int -= 360;
	}
	
	add_assoc_long(array, "rotation_angle", rotate_int);
	
	add_assoc_long(array, "matrix_width", dmtxGetSymbolAttribute(DmtxSymAttribSymbolRows, region->sizeIdx) );
	add_assoc_long(array, "matrix_height", dmtxGetSymbolAttribute(DmtxSymAttribSymbolCols, region->sizeIdx) );

	add_assoc_long(array, "data_regions_horizontal", dmtxGetSymbolAttribute(DmtxSymAttribHorizDataRegions, region->sizeIdx));
	add_assoc_long(array, "data_regions_vertical", dmtxGetSymbolAttribute(DmtxSymAttribVertDataRegions, region->sizeIdx));

	add_assoc_long(array, "interleaved_blocks", dmtxGetSymbolAttribute(DmtxSymAttribInterleavedBlocks, region->sizeIdx));

	MAKE_STD_ZVAL(edges);
	array_init(edges);
	
	_add_assoc_pixel(edges, "left", region->leftLoc);
	_add_assoc_pixel(edges, "bottom", region->bottomLoc);
	_add_assoc_pixel(edges, "top", region->topLoc);
	_add_assoc_pixel(edges, "right", region->rightLoc);	

	MAKE_STD_ZVAL(bounds);
	array_init(bounds);
	
	_add_assoc_pixel(bounds, "bound_min", region->boundMin);
	_add_assoc_pixel(bounds, "bound_max", region->boundMax);
	
	/* Add edges and bounds to return array */
	add_assoc_zval(array, "edges", edges);
	add_assoc_zval(array, "bounds", bounds);
	
	return array;
}
/* }}} */

/* {{{ proto array dmtxRead::getInfo([int scan_gap, int corrections, int type])
	Fetches the information from the image */
PHP_METHOD(dmtxread, getinfo)
{
	php_dmtx_read_object *intern;
	long scan_gap = 1, corrections = DmtxUndefined, type = PHP_DMTX_MATRIX;
	DmtxTime timeout;
	
	long i, j, start, limit;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|lbla", &scan_gap, &corrections, &type) == FAILURE) {
		return;
	}

	if (scan_gap <= 0) {
		PHP_DMTX_THROW_GENERIC_EXCEPTION("The scan gap needs to be larger than zero");
	}

	intern = (php_dmtx_read_object *)zend_object_store_get_object(getThis() TSRMLS_CC);

    
	/* init the return value as an array */
	array_init(return_value);

	if (intern->options.start <= 0) {
		start = 0;
		MagickResetIterator(intern->magick_wand);
	} else {
		/* Set iterator to one before because iteration uses MagickNextImage */
		if (MagickSetIteratorIndex(intern->magick_wand, (intern->options.start - 1)) == MagickFalse) {
			MagickResetIterator(intern->magick_wand);
			start = 0;
		} else {
			start = intern->options.start;
		}
	}

	if (intern->options.limit < 0) {
		limit = MagickGetNumberImages(intern->magick_wand);
	} else {
		limit = intern->options.limit;
	}

	/* Loop through all pages */
	j = 0;
	for (i = start; MagickNextImage(intern->magick_wand) != MagickFalse; i++) {
		DmtxImage *image;
		DmtxDecode *decode;
		zval *current_page;

		/* Honor the limit */
		if (j >= limit) {
			break;
		}
		j++;
		
		/* Get image */
		image = php_create_dmtx_image_from_wand(intern->magick_wand TSRMLS_CC);

		if (!image) {
			continue;
		}
		
		decode = dmtxDecodeCreate(image, intern->options.shrink);
		
		if (!decode) {
			efree(image->pxl);
			dmtxImageDestroy(&image);
			continue;
		}
		
		dmtxDecodeSetProp(decode, DmtxPropScanGap, scan_gap);

		/**
		 * Added encoding options select
		 * @author Mykhailo Stadnyk <mikhus@gmail.com>
		 */
		dmtxDecodeSetProp(decode, DmtxPropScheme, intern->options.scheme);
		
		if (intern->scan_region.active) {
			
			if (intern->scan_region.x_max > image->width) {
				intern->scan_region.x_max = image->width;
			}

			if (intern->scan_region.y_max > image->height) {
				intern->scan_region.y_max = image->height;
			}			
			
			if (dmtxDecodeSetProp(decode, DmtxPropXmin, intern->scan_region.x_min) == DmtxFail ||
				dmtxDecodeSetProp(decode, DmtxPropXmax, intern->scan_region.x_max) == DmtxFail ||
				dmtxDecodeSetProp(decode, DmtxPropYmin, intern->scan_region.y_min) == DmtxFail ||
				dmtxDecodeSetProp(decode, DmtxPropYmax, intern->scan_region.y_max) == DmtxFail) {
					
					PHP_DMTX_THROW_GENERIC_EXCEPTION("Failed to set scan region");
				}
		}
		
		if (dmtxDecodeSetProp(decode, DmtxPropSymbolSize, intern->options.symbol) == DmtxFail) {
			efree(image->pxl);
			dmtxImageDestroy(&image);
			PHP_DMTX_THROW_GENERIC_EXCEPTION("Failed to set symbol");
		}
		
		/* Current page is an array of regions */
		MAKE_STD_ZVAL(current_page);
		array_init(current_page);

		/* Loop through all regions on the page */
		for (;;) {
			DmtxRegion *region;
			zval *region_array;

			/* Let's see how it goes */
			if (intern->options.timeout_ms >= 0) {
            	timeout = dmtxTimeAdd(dmtxTimeNow(), intern->options.timeout_ms);
				region = dmtxRegionFindNext(decode, &timeout);
			} else {
				region = dmtxRegionFindNext(decode, NULL);
			}

			/* No region found */
			if (!region) {
				break;
			}
			
			/* Convert region info to php array */
			region_array = _php_dmtx_region_to_array(decode, region, type, corrections TSRMLS_CC);
			
			if (region_array) {
				add_next_index_zval(current_page, region_array);
			}
			dmtxRegionDestroy(&region);
		}
		add_index_zval(return_value, i, current_page);
		efree(image->pxl);
		dmtxImageDestroy(&image);
		dmtxDecodeDestroy(&decode);
	}
	return;
}
/* }}} */

/* {{{ proto void dmtxWrite::__construct([string message])
	Constructs the dmtxWrite object */
PHP_METHOD(dmtxwrite, __construct)
{
	php_dmtx_write_object *intern;
	char *message = NULL;
	int message_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|s!", &message, &message_len) == FAILURE) {
		return;
	}
	
	if (!message || message_len == 0) {
		return;
	}

	if (message_len >= DMTXWRITE_BUFFER_SIZE) {
		PHP_DMTX_THROW_GENERIC_EXCEPTION("The message is larger than the maximum allowed size");
	}

	intern = (php_dmtx_write_object *)zend_object_store_get_object(getThis() TSRMLS_CC);	
	memset(intern->message, '\0', DMTXWRITE_BUFFER_SIZE);
	strncpy(intern->message, message, message_len);
	intern->message_len = message_len;
	intern->scheme = PhpDmtxSchemeBase256; // added
	return;
}
/* }}} */

/* {{{ proto bool dmtxWrite::setMessage([string message])
	Sets the message on the object */
PHP_METHOD(dmtxwrite, setmessage)
{
 	php_dmtx_write_object *intern;
	char *message;
	int message_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &message, &message_len) == FAILURE) {
		return;
	}

	if (message_len >= DMTXWRITE_BUFFER_SIZE) {
		PHP_DMTX_THROW_GENERIC_EXCEPTION("The message is larger than the maximum allowed size");
	}

	intern = (php_dmtx_write_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
	
	memset(intern->message, '\0', DMTXWRITE_BUFFER_SIZE);
	strncpy(intern->message, message, message_len);
	intern->message_len = message_len;

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool dmtxWrite::setScheme([integer scheme])
	Sets the encoding scheme on the object
*/
PHP_METHOD(dmtxwrite, setscheme)
{
 	php_dmtx_write_object *intern;
	long scheme;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &scheme) == FAILURE) {
		return;
	}

	if (scheme < PhpDmtxSchemeAscii || scheme > PhpDmtxSchemeBase256) {
		PHP_DMTX_THROW_GENERIC_EXCEPTION("Invalid encoding scheme assigned!");
	}

	intern = (php_dmtx_write_object *)zend_object_store_get_object(getThis() TSRMLS_CC);

	intern->scheme = scheme;

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool dmtxWrite::getScheme()
	Returns encoding scheme on the object
*/
PHP_METHOD(dmtxwrite, getscheme)
{
	php_dmtx_write_object *intern;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "") == FAILURE) {
		return;
	}

	intern = (php_dmtx_write_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
	RETURN_LONG(intern->scheme);
}
/* }}} */

/* {{{ proto bool dmtxWrite::save(string filename[, int symbol, int type])
	Saves the message into a file */
PHP_METHOD(dmtxwrite, save)
{
 	php_dmtx_write_object *intern;
	char *filename;
	int filename_len, status;
	long symbol = DmtxSymbolSquareAuto, width, height, type = PHP_DMTX_MATRIX;
	DmtxEncode *encode;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|ls!", &filename, &filename_len, &symbol, &type) == FAILURE) {
		return;
	}

	intern = (php_dmtx_write_object *)zend_object_store_get_object(getThis() TSRMLS_CC);

	if (intern->message_len == 0) {
		PHP_DMTX_THROW_GENERIC_EXCEPTION("The object does not contain a message");
	}

	if (strlen(filename) >= MAXPATHLEN) {
		PHP_DMTX_THROW_GENERIC_EXCEPTION("The filename exceeds the maximum allowed length");
	}

	encode = dmtxEncodeCreate();
	
	/* Pack as RGB */
	dmtxEncodeSetProp(encode, DmtxPropPixelPacking, DmtxPack24bppRGB);
	dmtxEncodeSetProp(encode, DmtxPropSizeRequest, symbol);

	/**
	 * Sets selected encoding scheme
	 */
	dmtxEncodeSetProp(encode, DmtxPropScheme, intern->scheme);
	
	if (type == PHP_DMTX_MOSAIC) {
		status = dmtxEncodeDataMosaic(encode, intern->message_len, (unsigned char *)intern->message);
	} else {
		status = dmtxEncodeDataMatrix(encode, intern->message_len, (unsigned char *)intern->message);
	}

	if (status == DmtxFail) {
		dmtxEncodeDestroy(&encode);
		PHP_DMTX_THROW_GENERIC_EXCEPTION("Failed to encode the image");
	}

	width  = dmtxImageGetProp(encode->image, DmtxPropWidth);
    height = dmtxImageGetProp(encode->image, DmtxPropHeight);

	/* Clear if previous images */
	if (MagickGetNumberImages(intern->magick_wand) > 0) {
		ClearMagickWand(intern->magick_wand);
	}

	/* Import the pixels */
	if (MagickConstituteImage(intern->magick_wand, width, height, "RGB", CharPixel, encode->image->pxl) == MagickFalse) {
		dmtxEncodeDestroy(&encode);
		PHP_DMTX_THROW_IMAGE_EXCEPTION(intern->magick_wand, "Failed to import image");
	}

	if (MagickWriteImage(intern->magick_wand, filename) == MagickFalse) {
		dmtxEncodeDestroy(&encode);
		PHP_DMTX_THROW_GENERIC_EXCEPTION("Failed to write image");
	}

	dmtxEncodeDestroy(&encode);
	RETURN_TRUE;
}

/* Initialization */
static void php_dmtx_object_free_storage(void *object TSRMLS_DC)
{
	php_dmtx_object *intern = (php_dmtx_object *)object;
	
	if (!intern) {
		return;
	}

	zend_object_std_dtor(&intern->zo TSRMLS_CC);
	efree(intern);
}

static void php_dmtx_read_object_free_storage(void *object TSRMLS_DC)
{
	php_dmtx_read_object *intern = (php_dmtx_read_object *)object;
	
	if (!intern) {
		return;
	}

	intern->magick_wand = DestroyMagickWand(intern->magick_wand);
	zend_object_std_dtor(&intern->zo TSRMLS_CC);
	efree(intern);
}

static void php_dmtx_write_object_free_storage(void *object TSRMLS_DC)
{
	php_dmtx_write_object *intern = (php_dmtx_write_object *)object;
	
	if (!intern) {
		return;
	}

	intern->magick_wand = DestroyMagickWand(intern->magick_wand);
	zend_object_std_dtor(&intern->zo TSRMLS_CC);
	efree(intern);
}

static zend_object_value php_dmtx_object_new(zend_class_entry *class_type TSRMLS_DC)
{
	zval *tmp;
	zend_object_value retval;
	php_dmtx_object *intern;
	
	/* Allocate memory for it */
	intern = emalloc(sizeof(php_dmtx_read_object));
	memset(&intern->zo, 0, sizeof(php_dmtx_read_object));
	
	zend_object_std_init(&intern->zo, class_type TSRMLS_CC);
#if PHP_VERSION_ID < 50399
	zend_hash_copy(intern->zo.properties, &class_type->default_properties, (copy_ctor_func_t) zval_add_ref,(void *) &tmp, sizeof(zval *));
#else
	object_properties_init((zend_object*) &(intern->zo.properties), class_type);
#endif
	
	retval.handle = zend_objects_store_put(intern, NULL, (zend_objects_free_object_storage_t) php_dmtx_object_free_storage, NULL TSRMLS_CC);
	retval.handlers = (zend_object_handlers *) &php_dmtx_object_handlers;
	return retval;
}

static zend_object_value php_dmtx_read_object_new(zend_class_entry *class_type TSRMLS_DC)
{
	zval *tmp;
	zend_object_value retval;
	php_dmtx_read_object *intern;
	
	/* Allocate memory for it */
	intern = emalloc(sizeof(php_dmtx_read_object));
	memset(&intern->zo, 0, sizeof(php_dmtx_read_object));

	intern->magick_wand = NewMagickWand();
	
	intern->options.timeout_ms = -1;
	intern->options.start = -1;
	intern->options.limit = -1;
	intern->options.symbol = DmtxSymbolShapeAuto;
	intern->options.shrink = 1;
	intern->options.scheme = PhpDmtxSchemeBase256;
	
	intern->scan_region.x_min = 0;
	intern->scan_region.x_max = 0;
	
	intern->scan_region.y_min = 0;
	intern->scan_region.y_max = 0;
	
	intern->scan_region.active = 0;

	zend_object_std_init(&intern->zo, class_type TSRMLS_CC);
#if PHP_VERSION_ID < 50399
	zend_hash_copy(intern->zo.properties, &class_type->default_properties, (copy_ctor_func_t) zval_add_ref,(void *) &tmp, sizeof(zval *));
#else
	object_properties_init((zend_object*) &(intern->zo.properties), class_type);
#endif
	
	retval.handle = zend_objects_store_put(intern, NULL, (zend_objects_free_object_storage_t) php_dmtx_read_object_free_storage, NULL TSRMLS_CC);
	retval.handlers = (zend_object_handlers *) &php_dmtx_read_object_handlers;
	return retval;
}

static zend_object_value php_dmtx_write_object_new(zend_class_entry *class_type TSRMLS_DC)
{
	zval *tmp;
	zend_object_value retval;
	php_dmtx_write_object *intern;
	
	/* Allocate memory for it */
	intern = emalloc(sizeof(php_dmtx_write_object));
	memset(&intern->zo, 0, sizeof(php_dmtx_write_object));
	intern->magick_wand = NewMagickWand();

	memset(intern->message, '\0', DMTXWRITE_BUFFER_SIZE);
	intern->message_len = 0;
	intern->scheme = PhpDmtxSchemeBase256;

	zend_object_std_init(&intern->zo, class_type TSRMLS_CC);
#if PHP_VERSION_ID < 50399
	zend_hash_copy(intern->zo.properties, &class_type->default_properties, (copy_ctor_func_t) zval_add_ref,(void *) &tmp, sizeof(zval *));
#else
	object_properties_init((zend_object*) &(intern->zo.properties), class_type);
#endif
	retval.handle = zend_objects_store_put(intern, NULL, (zend_objects_free_object_storage_t) php_dmtx_write_object_free_storage, NULL TSRMLS_CC);
	retval.handlers = (zend_object_handlers *) &php_dmtx_write_object_handlers;
	return retval;
}

PHP_MINIT_FUNCTION(dmtx)
{
	MagickWandGenesis();

	zend_class_entry ce;
	memcpy(&php_dmtx_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	memcpy(&php_dmtx_read_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	memcpy(&php_dmtx_write_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));

	/*
		Initialize exceptions (dmtx exception)
	*/
	INIT_CLASS_ENTRY(ce, PHP_DMTX_EXCEPTION_SC_NAME, NULL);
	php_dmtx_exception_class_entry = zend_register_internal_class_ex(&ce, zend_exception_get_default(TSRMLS_C), NULL TSRMLS_CC);
	php_dmtx_exception_class_entry->ce_flags |= ZEND_ACC_FINAL;
	
	/*
		Initialize the class (dmtx). This class holds the constants
	*/
	INIT_CLASS_ENTRY(ce, PHP_DMTX_SC_NAME, php_dmtx_class_methods);
	ce.create_object = php_dmtx_object_new;
	php_dmtx_sc_entry = zend_register_internal_class(&ce TSRMLS_CC);

	/* Register constants */
	PHP_DMTX_REGISTER_CONST_LONG("SYMBOL_SHAPE_AUTO", DmtxSymbolShapeAuto);
	PHP_DMTX_REGISTER_CONST_LONG("SYMBOL_SQUARE_AUTO", DmtxSymbolSquareAuto);
	PHP_DMTX_REGISTER_CONST_LONG("SYMBOL_SQUARE_COUNT", DmtxSymbolSquareCount);
	PHP_DMTX_REGISTER_CONST_LONG("SYMBOL_RECT_AUTO", DmtxSymbolRectAuto);
	PHP_DMTX_REGISTER_CONST_LONG("SYMBOL_RECT_COUNT", DmtxSymbolRectCount);
	
	/* Different types */
	PHP_DMTX_REGISTER_CONST_LONG("TYPE_MATRIX", PHP_DMTX_MATRIX);
	PHP_DMTX_REGISTER_CONST_LONG("TYPE_MOSAIC", PHP_DMTX_MOSAIC);

	/**
	 * different schemes
	 * @author Mykhailo Stadnyk <mikhus@gmail.com>
	 */
	PHP_DMTX_REGISTER_CONST_LONG("SCHEME_ASCII", PhpDmtxSchemeAscii);
	PHP_DMTX_REGISTER_CONST_LONG("SCHEME_C40", PhpDmtxSchemeC40);
	PHP_DMTX_REGISTER_CONST_LONG("SCHEME_TEXT", PhpDmtxSchemeText);
	PHP_DMTX_REGISTER_CONST_LONG("SCHEME_X12", PhpDmtxSchemeX12);
	PHP_DMTX_REGISTER_CONST_LONG("SCHEME_EDITFACT", PhpDmtxSchemeEdifact);
	PHP_DMTX_REGISTER_CONST_LONG("SCHEME_BASE256", PhpDmtxSchemeBase256);
	
	/*
		Initialize the class (dmtx read)
	*/
	INIT_CLASS_ENTRY(ce, PHP_DMTX_READ_SC_NAME, php_dmtx_read_class_methods);
	ce.create_object = php_dmtx_read_object_new;
	php_dmtx_read_sc_entry = zend_register_internal_class(&ce TSRMLS_CC);

	/*
		Initialize the class (dmtx write)
	*/
	INIT_CLASS_ENTRY(ce, PHP_DMTX_WRITE_SC_NAME, php_dmtx_write_class_methods);
	ce.create_object = php_dmtx_write_object_new;
	php_dmtx_write_sc_entry = zend_register_internal_class(&ce TSRMLS_CC);

	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(dmtx)
{
	MagickWandTerminus();
	return(SUCCESS);
}

PHP_MINFO_FUNCTION(dmtx)
{
	php_info_print_table_start();
	php_info_print_table_row(2, "dmtx module", "enabled");
	php_info_print_table_row(2, "dmtx module version", PHP_DMTX_EXTVER);
	php_info_print_table_end();
}

#if PHP_VERSION_ID < 50399
static function_entry php_dmtx_functions[] =
#else
static zend_function_entry php_dmtx_functions[] =
#endif
{
	{ NULL, NULL, NULL }
};

zend_module_entry dmtx_module_entry =
{
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	PHP_DMTX_EXTNAME,
	php_dmtx_functions,			/* Functions */
	PHP_MINIT(dmtx),			/* MINIT */
	PHP_MSHUTDOWN(dmtx),		/* MSHUTDOWN */
	NULL,						/* RINIT */
	NULL,						/* RSHUTDOWN */
	PHP_MINFO(dmtx),			/* MINFO */
#if ZEND_MODULE_API_NO >= 20010901
	PHP_DMTX_EXTVER,
#endif
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_DMTX
ZEND_GET_MODULE(dmtx)
#endif


