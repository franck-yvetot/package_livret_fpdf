<?php
// package
$package = 'livret_fpdf';

// prefix for dml views .record, .list, .search.
$base_data = 'livret_fpdf';

// data sources (optional)
$datasources = array(
	// WORKFLOW
	'header'   => 'livret_pdf.record@livret_fpdf', // state header

	// ADD
	'new'	   => 'livret_pdf.add@livret_fpdf',    // new record form
	'add_process'   => 'livret_pdf.record@livret_fpdf',  // save new form

	// EDIT
	'record'   => 'livret_pdf.record@livret_fpdf',       // default record form
	'edit_process'   => 'livret_pdf.record@livret_fpdf', // save edited form

	// LIST
	'search'   => 'livret_pdf.search@livret_fpdf', // search form
	'excel'    => 'livret_pdf.excel@livret_fpdf',  // excel list export
	'list'	   => 'livret_pdf.list@livret_fpdf',   // list
);

// strings name
$strings = 'offer_pdf@livret_fpdf';

// permissions foir the application
$permissions='default';

// same role as in www .php file
$main_role='editor';

$this->ctxt->setProperty('app.form.css',<<<ECSS
	/* CSS RULES here */
ECSS
);

$this->ctxt->setProperty('app.form.js',<<<EJS
	/* js code */
EJS
);

// form definition
$forms = array(
	'record' => array(
		'tabs' => array(
			'Informations' => array(
				'title' => 'Informations', 
				'fields' => array('*')
			),
		)
	)
);

// optional views 
$views = array
(
	'list' => 'workflow.list_page@workflow',
	'record' => 'workflow.xedit_generic_attachments3@workflow',
	'excel' => 'exports.excel.records',
);

// base application
$app_pre = isset($application) ? ($application.':') : '';

// extension liste des boutons de la liste
if(!isset($listActions))
	$listActions=array();

// classes des icones à appliquer aux boutons
$icons = array(
	'add'=>'si-add',
	'edit'=>'si-page-white-edit',
	'delete'=>'si-delete',
	'submit'=>'si-disk',
	'save'=>'si-disk',
	'save_continue'=>'si-disk',
	'export_excel'=>'si-page-white-excel');

// nom des vues DML à appliquer par les opérations
if(!isset($datasources))
	$datasources = array(
		'header'   => $base_data.'.record@'.$package,
		'new'	   => $base_data.'.add@'.$package,
		'record'   => $base_data.'.record@'.$package,
		'search'   => $base_data.'.search@'.$package,
		'excel'    => $base_data.'.excel@'.$package,
		'list'	   => $base_data.'.list@'.$package,
	);

if(!isset($views))
{
	$views = array
	(
		'list' => 'workflow.list_page@workflow',
		'record' => 'workflow.edit_page@workflow',
		'excel' => 'exports.excel.records',
	);
}

if(!isset($forms))
	$forms = array(
	'record'=>array()
	);

// $onLoad($workflow,$user,$record,$thisTask,$method)
if(!isset($onLoad))
	$onLoad = null;

// $onLoadForm($workflow,$user,$record,$thisTask,$method)
if(!isset($onLoadForm))
	$onLoadForm = null;

// $onChange($workflow,$user,$changes,$old_record,$record,$thisTask)
if(!isset($onChange))
	$onChange = null;

// $onSave($workflow,$user,$record,$thisTask,$method)
if(!isset($onSave))
	$onSave = null;

if(!isset($showSearch))
	$showSearch = 'no';

$properties=array
(
	// ========================= CRUD APPLICATION ==================
	'application' => array
	(
		// application descriptor for role = magasin
		$main_role => array
		(
			'meta' => array
			(
				'description' => ('Edit items'),
				'strings' => $strings,

				// data descriptors
				'data'  => $datasources,

				// default display views 
				'views' => $views,
				'icons'=>$icons,
				'forms' => $forms,

				// opérations pouvant être appellées depuis la liste, pour chaque entrée
				'list_operations' => array('edit'=>array('nxo'=>$app_pre.'edit'),'delete'=>array('nxo'=>$app_pre.'delete','confirm'=>true)),

				// opérations par défaut
				'operation_init'=>$app_pre.'print_pdf',
				'title' => 'list records', 

				// default tasks executed for loading a state
				// 'state_loader'  => 'Workflow_StateLoader',
				'onload'  => array(),

				// tasks executed before executing an action : load submitted form
				'onBeforeAction'  => array(),
				
				'permissions' => $permissions
			),
			
			// OPERATIONS DE L'APPLICATION
			'operations' => array(

				'print_pdf' => array(

					'tasks' => array(
						'offers_pdf'=>array(
							// 'json_url'=>$_SERVER['HTTP_HOST'].'/livret/api/livret.php?u=pmadmin&p=test&op=pages',
							'offers_json_url'=>$_SERVER['HTTP_HOST'].'/livret/api/livret.php?op=pages',
							'doctors_json_url'=>$_SERVER['HTTP_HOST'].'/livret/api/livret.php?op=doctors',	
						),
					),
				),
				'print_pdf_test' => array(

					'tasks' => array(
						'offers_pdf_test'=>array(
							// 'json_url'=>$_SERVER['HTTP_HOST'].'/livret/api/livret.php?u=pmadmin&p=test&op=pages',
							'offers_json_url'=>$_SERVER['HTTP_HOST'].'/livret/api/livret.php?op=pages',
							'doctors_json_url'=>$_SERVER['HTTP_HOST'].'/livret/api/livret.php?op=doctors',
							
						),
					),
				),
			)
		)
	)
);

?>