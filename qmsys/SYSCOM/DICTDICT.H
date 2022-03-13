* DICTDICT.H
* Dictionary file layout keys
* Copyright (c) 2004, Ladybridge Systems, All Rights Reserved

$define DICT.TYPE            1  ;* C, D, I, PH, X
$define DICT.LOC             2  ;* Field number for D type items
$define DICT.ITYPE.SOURCE    2  ;* Expression for C and I type items
$define DICT.CONV            3  ;* Conversion code (optional)
$define DICT.DISPLAY.NAME    4  ;* Report column heading (optional)
$define DICT.FORMAT          5  ;* Report format specification
$define DICT.S.M             6  ;* Single/Multivalued
$define DICT.ASSOC           7  ;* Association name (optional)
$define DICT.USER            8  ;* For user use - Not processed by QM
* Fields 9 - 14 are not currently assigned but should not be used by users
* as they may be assigned in future releases.
$define DICT.SYS.INFO       15  ;* For internal use by QM
$define DICT.ITYPE.OBJECT   16  ;* Compiled I-type code (multiple fields)


* A and S type dictionary fields
* QM compiles correlatives in the same way as I-types, leaving the object
* code in field 16 onwards.

$define DICT.A.LOC           2  ;* Field number
$define DICT.A.DISPLAY.NAME  3  ;* Report column heading (optional)
$define DICT.A.ASSOC         4  ;* Association
$define DICT.A.CONV          7  ;* Conversion code (optional)
$define DICT.A.CORRELATIVE   8  ;* Correlative code
$define DICT.A.JUSTIFY       9  ;* Justification
$define DICT.A.WIDTH        10  ;* Width
