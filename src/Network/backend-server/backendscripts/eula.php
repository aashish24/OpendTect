<?php
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer
 Date:          Jul 2016
________________________________________________________________________

Displays the EULA in HTML. It generated html code that can be viewed in a browser
or be printed into a PDF file.

Usage uela.php?format=crm

Format can be either:
    'view' - two columns with normal text size.
    'pdf'  - two columns with small text size
    'crm'  - single column. For copy paste of HTML to pdf template in crm
-*/

$format = 'view';
if ( array_key_exists( "format", $_GET ) )
    $format = $_GET['format'];

$twocolumns = $format == 'pdf' || $format == 'view';

$marginamount = $twocolumns ? "1em" : "5em";
$fontsize = $format == 'view' ? '' : 'font-size: xx-small; ';
$subtitlefontsize = $format == 'view' ? '' : 'font-size: x-small; ';

$margin = "margin-left: $marginamount; margin-right: $marginamount;";

$DefaultStyle = "$margin ".
		"text-align: justify; ".
		$fontsize.
		"margin-bottom: 0em; ".
		"margin-top: 0em;";

$Level1Style = "$margin ".
		$fontsize.
		"font-weight: bold";

$Level2Style = "$margin ".
		"text-align: justify; ".
		$fontsize.
		"margin-bottom: 0em; ".
		"margin-top: 0em; ".
		"padding-left: 1em; ".
		"text-indent: -1em;";

$Level3Style = "$margin ".
		"text-align: justify; ".
		$fontsize.
		"margin-bottom: 0em; ".
		"margin-top: 0em; ".
		"padding-left: 2em; ".
		"text-indent: -1em;";

$TitleStyle = "text-align: center; font-weight: bold; $margin";

$SubTitleStyle = "text-align: center; $subtitlefontsize $margin";


echo '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN"'.
     '"http://www.w3.org/TR/html4/strict.dtd">'.
    '<html>'.
    '<head>'.
    '<meta http-equiv="Content-Type" content="text/html; charset=utf-8">';

$eulatext = file_get_contents( dirname(__FILE__)."/../../../../doc/eula.txt" );
$eulaarray = explode( "\n", $eulatext );

$linenr = 0;

foreach ( $eulaarray AS $line )
{
    $linearray = explode( " ", $line );
    $style = $DefaultStyle;

    if ( $linenr==0 )
    {
	$style = $TitleStyle;
	echo '<title>'.$line.'</title></head><body>';
	echo "\n\n\n\n";
    }

    else if ( $linenr==1 )
    {
	$style = $SubTitleStyle;
    }

    else if ( preg_match( "/[0-9]+\./m", $linearray[0] ) == 1 )
	$style = $Level1Style;
    else if ( preg_match( "/[A-Z]+\.+/m", $linearray[0] ) == 1 )
	$style = $Level2Style;
    else if ( preg_match( "/[a-z]+\.+/m", $linearray[0] ) == 1 )
	$style = $Level3Style;

    echo '<p style="'.$style.'">'.htmlspecialchars($line)."</p>\n";

    if ( $twocolumns && $linenr==1 )
    {
	echo '<div style="'.
    	     '-webkit-column-count: 2; '.
    	     '-webkit-column-gap: 0em; '.
    	     '-moz-column-count: 2; '.
    	     '-moz-column-gap: 0em; '.
    	     'column-count:2; '.
    	     'column-gap: 0em;">';
    }

    $linenr++;
}

if ( $twocolumns )
    echo "<div>";

echo "\n\n\n\n\n</body></html>\n";