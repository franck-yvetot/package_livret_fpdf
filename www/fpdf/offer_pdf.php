<?php
/* file automatically generated from NXL source [d:/webs/access/generis_2016/packages/livret_fpdf/nxl/offer_pdf.nxl]. Do not edit it. */
		
$loginPerm='default';
$loginService='user';
global $NX_MIME;
$NX_MIME='text/html';
require_once('../../nx/NxSite.inc');

include_once GENERIS_PACKAGES."workflow/lib/workflow/ExecuteApp2.inc";

/** MODIFIER CES VALEURS (si besoin)**/
$package = 'livret_fpdf';
$app  = 'offer_pdf';
$role = 'editor';
$skin = 'lightsky';
/** FIN MODIF **/

// nom application et package, par defaut, on utilise le nom du fichier comme application

// use generic application application bootstrap that loads the application from the package
$page=new ExecuteApp2(
	array(
		'app.id' => "$app@$package",
		'app.role' => $role,
		'app.defaultOperation'=>'list',
		'nx.package' => $package,
		'application.dir'=>'application',
		'page.skin' => $skin,
	),
	$siteDesc);

/* call the application */
try {
$page->run();
}
catch(Exception $e) {
	echo $e;
}
?>