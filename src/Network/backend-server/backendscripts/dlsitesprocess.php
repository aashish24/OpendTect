<?php
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer
 Date:		Jun 2016
________________________________________________________________________

-*/

include_once( 'dlsitesdb.php' );
include_once( 'dlsitessystemid.php' );


function store_entry( $db, $tablename, $id, $platform, $country, $nrcpu, $mem )
{
    global $DLSITES_TABLE_PREFIX;
    $tablename = $DLSITES_TABLE_PREFIX.$tablename;

    $query = "CREATE TABLE IF NOT EXISTS `$tablename` ("
      	    ."`id` BIGINT UNSIGNED NOT NULL, "
      	    ."`platform` VARCHAR(6) NOT NULL, "
      	    ."`country` VARCHAR(10) DEFAULT NULL, "
      	    ."`memory` BIGINT UNSIGNED DEFAULT NULL, "
      	    ."`cpu` INT(8) DEFAULT NULL "
    	    .") ENGINE=InnoDB";

    if ( $db->query( $query ) === false )
    {
	echo "Cannot create table $tablename\n";
	return false;
    }

    $query = "INSERT INTO `$tablename` (`id`,`platform`,`country`,`memory`,`cpu` ) "
	    ."VALUES ( '$id', '$platform', '$country', $mem, $nrcpu )";

    if ( $db->query( $query )=== false )
    {
	echo "Cannot insert item into table $tablename\n";
	return false;
    }

    return true;
}

$mysqli = connect_dlsitesdb();

$rootdir = ".";
if ( array_key_exists( 'DOCUMENT_ROOT', $_SERVER ) && $_SERVER['DOCUMENT_ROOT']!='' )
{
    $rootdir = $_SERVER['DOCUMENT_ROOT'];
}

$inputdir = $rootdir."/".$DLSITES_OUTPUT_DIR."/";
$archivedir = $rootdir."/".$DLSITES_ARCHIVE_DIR."/";

foreach(glob($inputdir."/*.txt", GLOB_NOSORT) as $file)   
{  
    if ( !file_exists( $file ) ) //It may be temporary renamed
	continue;

    $inputdata = file_get_contents( $file );

    if ( $inputdata===false )
    {
	echo "Cannot read $file\n";
	continue;
    }

    $inputarray = explode( "\n", $inputdata );

    foreach ( $inputarray as $input )
    {
	$listing = (array) json_decode( $input );
	if ( empty( $listing ) )
	    continue;

	if ( !array_key_exists( 'address', $listing ) )
	{
	    echo "Cannot read address in $file. Skipping\n";
	    continue;
	}

	if ( !array_key_exists( 'country', $listing ) )
	{
	    $ipnumber = $listing['address'];
	    $iplookup = file_get_contents( "http://api.db-ip.com/addrinfo?addr=$ipnumber&api_key=6657b916b375e9beb92a17a5f9ec0b232715bec8" );
	    if ( $iplookup!==false )
	    {
		$iplookuparr = (array) json_decode( $iplookup );
		$country = '';
		if ( array_key_exists( 'country', $iplookuparr ) )
		    $country = $iplookuparr['country'];

		$listing['country'] = $country;
	    }
	}

	$machash = array_key_exists( 'i', $listing ) ? $listing['i'] : '';
	$platform = array_key_exists( 'p', $listing ) ? $listing['p'] : '';
	$country =  array_key_exists( 'country', $listing ) ? $listing['country'] : '';
	$nrcpu =  array_key_exists( 'c', $listing ) ? $listing['c'] : "NULL";
	$mem =  array_key_exists( 'm', $listing ) ? $listing['m'] : "NULL";

	if ( $platform!='' && $machash!='' )
	{
	    $timestamp = new DateTime( $listing['date'] );
	    $year_table_name = $timestamp->format( "Y" ); 
	    $month_table_name = $timestamp->format( "Y-m" );
	    $total_table_name = 'total';

	    $id = resolve_system_id( $mysqli, $machash, $platform, $nrcpu, $mem, true );

	    if ( !store_entry( $mysqli, $year_table_name, $id, $platform, $country, $nrcpu, $mem ) ||
		 !store_entry( $mysqli, $month_table_name, $id, $platform, $country, $nrcpu, $mem ) ||
		 !store_entry( $mysqli, $total_table_name, $id, $platform, $country, $nrcpu, $mem ) )
	    {
		echo "Failure in storing entry from $file\n";
	    }
	}
    }

    $archivename = $archivedir.basename( $file );
    rename( $file, $archivename );
}
