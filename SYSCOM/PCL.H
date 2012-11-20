* PCL.H
* PCL function definitions
* Copyright (c) 2005, Ladybridge Systems, All Rights Reserved

deffun pcl.box(l,t,w,h,p,r) calling "!PCL" key 5
deffun pcl.copies(n) calling "!PCL" key 7
deffun pcl.cursor(x, y) calling "!PCL" key 2
deffun pcl.duplex(mode) calling "!PCL" key 13
deffun pcl.font(typeface) calling "!PCL" key 6
deffun pcl.font.weight(wgt) calling "!PCL" key 14
deffun pcl.hline(x, y, length, line.width) calling "!PCL" key 3
deffun pcl.left.margin(col) calling "!PCL" key 12
deffun pcl.orientation(layout) calling "!PCL" key 9
deffun pcl.paper.size(size) calling "!PCL" key 8
deffun pcl.reset() calling "!PCL" key 1
deffun pcl.restore.csr() calling "!PCL" key 11
deffun pcl.save.csr() calling "!PCL" key 10
deffun pcl.vline(x, y, length, line.width) calling "!PCL" key 4


* ======================================================================
* Layout of SYSCOM $PCLDATA record
* Users may add new values but should not remove existing ones as some parts
* of QM may rely on these being present.

$define PCL.FONT.NAME         1   ;* Font name and...
$define PCL.FONT.NO           2   ;* ...corresponding font number...
$define PCL.FONT.SPACING      3   ;* ...proportional spaced?

$define PCL.SYMBOL.SET        4   ;* Symbol set names and...
$define PCL.SYMBOL.SET.CODES  5   ;* ...corresponding codes

$define PCL.STYLE             6   ;* Style names and...
$define PCL.STYLE.NO          7   ;* ...corresponding codes

$define PCL.WEIGHT            8   ;* Stroke weights and...
$define PCL.WEIGHT.NO         9   ;* ...corresponding codes

$define PCL.SPACING          10   ;* Spacing names and...
$define PCL.SPACING.NO       11   ;* ...corresponding codes

$define PCL.PAPER.NAME       12   ;* Paper size names and...
$define PCL.PAPER.NO         13   ;* ...corresponding codes

$define PCL.ORIENTATION.NAME 14   ;* Page orientation names and...
$define PCL.ORIENTATION.NO   15   ;* ...corresponding codes

$define PCL.VALID.LPI        16   ;* Valid values for line spacing (lines per inch)
