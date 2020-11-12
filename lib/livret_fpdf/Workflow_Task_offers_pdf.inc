<?php

include_once(NX_LIB.'framework.inc');
require(NX_LIB.'fpdf/fpdf.php');
require(NX_LIB.'fpdi/fpdi.php');
require_once('curl_http_client.php');
// application task

class Workflow_Task_offers_pdf extends Workflow_Task
{
	function run($workflow, $user)
	{
		
		//DebugBreak();
		$url =$this->getParam('offers_json_url');

		if(isset($_REQUEST['soid']))
			$url=$url."&soid=".$_REQUEST['soid'];
		else
			$url=$url."&soid=0";

		$res=$this->getCurlRes($url,"Offres");

		if(!$res)
			return '';

		// decode
		$response = json_decode($res);
		if ($response === null && json_last_error() !== JSON_ERROR_NONE) 
		{
			return '';
		}

		//classement des pages
		$pdf = new LIVRET_PDF('P',"mm",array(115,220));
		$pdf->init();

		//cover page
		$pdf->coverInfos=$this->getCover($pdf,$response,$pdf_unity);
		//
		$pdf->generatePDFPage("cover");
		$establishment=$this->getEtablishment($response);
		$pdf->setEstablishment($establishment);

		
		//offers
		$doctors_db_list=$this->getDoctorsDB();

		$offers_list=$this->getOffers($response,$doctors_db_list);
		if($offers_list != null && !empty($offers_list) )
		{
			$pdf->offersList=$offers_list;
			$pdf->summaryInfos=$this->getSummary($pdf,$response,$offers_list);
			$pdf->generatePDFPage("summary");	
		}

		$pdf->emergencyInfos=$this->geEemergencyInfos($pdf,$response,$doctors_db_list);
		$pdf->generatePDFPage("emergency");

    // DebugBreak();
		$pdf->poleInfos=$this->getPoleInfos($pdf,$response);
		$pdf->generatePDFPage("pole");
	
		if($offers_list != null && !empty($offers_list) )
		{
			$pdf->generatePDFPage("index");
			foreach ($offers_list as $offer_page)
			{
				if($offer_page->display == true)
				{
					$pdf->setOffer($offer_page);
					$pdf->generatePDFPage("offer");
				}
			}
		}
		
		$pdf->generatePDFPage("last_cover");
			
		//$pdf->Output("Plaquette ".$establishment.".pdf",'D');
		$pdf->Output("I","Plaquette ".$pdf->header_title.".pdf");
	}

	function getCurlRes($url,$data_type)
	{
		
		//curl
		$oCurl= new Curl_HTTP_Client(false);

		$oCurl->attachPHPSession();

		// relacher la session pour ne pas bloquer l'API utilisant la même session
		// NB. en php : 1 seule requête peut utiliser la même session en même temps!!
		session_write_close();

		$res = $oCurl->fetch_url($url);

		if(!$res) {
			//echo ('Get '.$data_type.' failed');
			return false;
		}

		return $res;
	}

	function getOffers($response,$doctors_db_list)
	{
		$offers_list=array();
		$data = $response;

		$pages=$data->{'pages'};
		foreach ($pages as $key=>$page)
		{
			$page_type=$page->{"type"};
			if($page_type == "care_overview")
			{
				$offers=$pages[$key];
				break;
			}
		}

		$offers_pages=$offers->{"sub_pages"};
		foreach ($offers_pages as $page)
		{
			$page_data=$page->{"data"};
			$offer_obj=new OFFER ($page->{"oid"},$page->{"name"},$page_data->{"title"},$page->{"sort"},$page->{"selected"},$page_data->{"in_summary"},$page->{"summary_index"},$page_data->{"summary_alias"},$page_data->{"iconFile"});
			//doctors
			$offer_obj->setDoctors($this->getOfferDoctors($page_data));
			//sections
			$offer_sections=$this->getSections($page,$page_data);
			$offer_obj->setSections($offer_sections);
			//add to list
			$offers_list[$offer_obj->oid]=$offer_obj;
        }

		if($offers_list != null && !empty($offers_list) )
		{
			//ksort($offers_list);
			 usort($offers_list, array("OFFER", "compare"));
		}

		return $offers_list;
	}
	

	function getEtablishment($response)
	{
		$data = $response;

		$site=$data->{'site'};
		$establishment=$site->{"name"};
		return $establishment;
	}

	function getPoleInfos($pdf,$response)
	{
		$pole_info=null;
		$pole_info["pole_name"]=$this->getPoleName($response);

		$data = $response;

		$pages=$data->{'pages'};
		foreach ($pages as $key=>$page)
		{
			$page_type=$page->{"type"};
			if($page_type == "pole")
			{
				$pole=$pages[$key];
				break;
			}
		}

		$pole_data=$pole->{'data'};

		$pole_map_file=$pole_data->{'mapFile'};
		$pole_info["map_file"]=$pole_map_file;
		$pole_data=$pole_data->{'pole'};
		$pole_description=$pole_data->{'description'};
		if($pole_description == "Description du pôle (modifier ici en ligne...)")
			$pole_description="";
		$pole_description=html_entity_decode($pole_description, ENT_QUOTES | ENT_HTML5);
		$pole_description=iconv('UTF-8', 'windows-1252', stripslashes($pole_description));
		
		if($pole_data->offsetY)
			$pole_info["offsetY"] = 0+$pole_data->offsetY;
		else
			$pole_info["offsetY"] = 0;
        
		$pole_info["description"]=$pole_description;
		$pole_parts=$pole->{'parts'};
		$pole_sites=$pole_parts->{'sites'};
		$nb_beds=0;
		$sites=array();
		foreach($pole_sites as $site)
		{
			if($site->{"selected"})
			{
				$sites[]=$site;
				$nb_beds=$nb_beds+$site->{"nb_beds"};

			}
		}

		$pole_info["sites"]=$sites;
		$pole_info["nb_beds"]=$nb_beds;
		$pole_info["nb_hp"]=count($sites);

		return $pole_info;
	}

	function getPoleName($data)
	{
		$site=$data->{'site'};
		$pole=$site->{"pole_name"};
		return $pole;
	}

	function geEemergencyInfos($pdf,$response,$doctors_db_list)
	{
		$data = $response;
		$emergency_info=null;
		$pages=$data->{'pages'};
		foreach ($pages as $key=>$page)
		{
			$page_type=$page->{"type"};
			if($page_type == "urgencies")
			{
				$emergency=$pages[$key];
				$page_title=$page->{"name"};
				$emergency_info["page_title"]=$page_title;
				break;
			}
		}
		$emergency_data=$emergency->{'data'};
		$emergency_info["passages"]=$emergency_data->{'passages'};
		$emergency_info["passages_text"]=$emergency_data->{'passages_text'};
		$emergency_info["boxes"]=$emergency_data->{'boxes'};
		$emergency_info["box_text"]=$emergency_data->{'box_text'};
		$emergency_info["nb_bed_text"]=$emergency_data->{'nb_bed_text'};
		$emergency_info["nb_beds"]=$emergency_data->{'nb_beds'};

		$doctors=array();
		$doctors_oid=$emergency_data->{'praticiens'};
		if($doctors_db_list != null)
		{
			foreach ($doctors_db_list as $doctor)
			{
				foreach ($doctors_oid as $oid => $value)
				{
					if($doctor->oid == $oid && $value == TRUE)
					{
						$doctors[]=$doctor;
					}
				}
			}
		}

		$emergency_info["praticiens"]=$doctors;

		$emergency_parts=$emergency->{'parts'};
		$emergency_urgencies=$emergency_parts->{'urgencies'};

		$emergency_info["urgencies"]=array();
		$i=0;
		foreach ($emergency_urgencies as $key=>$element)
		{
			$type=gettype($element);
			if($type != "object")
				continue;

			$title=iconv('UTF-8', 'windows-1252', stripslashes($element->{'title'}));
			$description=iconv('UTF-8', 'windows-1252', stripslashes($element->{'description'}));
			$display_description=$element->{'with_description'};
			$contacts = $element->{'items'};

			$emergency_info["urgencies"][$i] = array(
				"title"=>$title,
				"description"=>$description,
				"display_description"=>$display_description,
				"contacts"=>$contacts,
			);
			$i++;
		}

		return $emergency_info;
	}

	function getSummary($pdf,$response,$offers_list)
	{
		$data = $response;
		
		$summary_info=null;
		$pages=$data->{'pages'};
		foreach ($pages as $key=>$page)
		{
			$page_type=$page->{"type"};
			if($page_type == "summary")
			{
				$summary=$pages[$key];
				break;
			}
		}

		$poleName = $data->site->pole_name;
		

		//$summary=$pages[1];
		$summary_data=$summary->{'data'};
		$summary_offers=$summary_data->{'icons'};
		$summary_coord=$summary_data->{'coord'};
		$summary_info["bed_number"]=$summary_data->{"bed_number"};
		$summary_info["bed_number_desc"]=iconv('UTF-8', 'windows-1252', stripslashes($summary_data->{"bed_number_desc"}));
		$summary_info["nb_doctors"]=$summary_data->{"nb_doctors"};
		$summary_info["summary_coord"]=$summary_coord;
		$summary_info["pole"]=$poleName;

		$summary_info["offers"]=$this->getSummaryOffers($offers_list,$summary_offers);

		if(isset($summary_data->and_also))
			$summary_info['and_also']=stripslashes($summary_data->and_also);
		else
			$summary_info['and_also']='';

		return $summary_info;
	}

	function getSummaryOffers($offers_list,$summary_offers)
	{
		//DebugBreak();
		$summaryOffers=array();
		$offers=array();
		
		foreach ($offers_list as $offer_page)
		{
			$inSammury=false;
			foreach ($summary_offers as $summary_offer)
			{
				if($offer_page->oid == $summary_offer->{"oid"})
				{
					$inSammury=true;
					$offer_page->summary_index=$summary_offer->{"summary_index"};
				}
			}
			//$offers[$offer_page->summary_index]=$offer_page;
			if($inSammury == true)
			{
				$offers[$offer_page->oid]=$offer_page;
			}
		}

		/*foreach ($offers_list as $offer_page)
		{
			if($offer_page->in_summary != NULL && $offer_page->in_summary == true)
			{
				//$offers[$offer_page->summary_index]=$offer_page;
				$offers[$offer_page->oid]=$offer_page;
			}
		}*/
		
		//ksort($offers);
		usort($offers, array("OFFER", "compare_summary_index"));
		$i=0;
		foreach ($offers as $offer)
		{
			$summaryOffers[$i]=$offer;
			$i++;
		}
		return $summaryOffers;
	}

	function getCover($pdf,$response,$pdf_unity)
	{
		$data = $response;

		$cover_info=null;
		$pages=$data->{'pages'};
		$site=$data->{'site'};
		foreach ($pages as $key=>$page)
		{
			$page_type=$page->{"type"};
			if($page_type == "cover")
			{
				$cover=$pages[$key];
				break;
			}
		}

		//$cover=$pages[0];
		$cover_data=$cover->{'data'};
		$site_cover=$site->{'cover'};

		$cover_info["margin_left"]=$margin_left;
		$cover_info["margin_right"]=$margin_right;
		$cover_info["title"]=iconv('UTF-8', 'windows-1252', stripslashes($cover_data->{"title"}));

		$cover_info["bg_x"]=$site_cover->{"bg_x"};
		$cover_info["bg_y"]=$site_cover->{"bg_y"};
		$cover_info["image"]=$site_cover->{"bgFile"};
		$cover_info["logo"]=$cover_data->{"logoFile"};
		$cover_info["logo2"]=$site_cover->{"logo2File"};
		if($site_cover->{"back_cover_text"})
			$cover_info["back_cover_text"]=$site_cover->{"back_cover_text"};

		$cover_info["subtitle"]=iconv('UTF-8', 'windows-1252', stripslashes($cover_data->{"subtitle"}));
		$cover_info["titre_edition"]=iconv('UTF-8', 'windows-1252', stripslashes($cover_data->{"titre_edition"}));
		
		$cover_info["title_x"]=$pdf_unity == "cm" ?$site_cover->{"title_x"}:$site_cover->{"title_x"}*10;

		$cover_info["title_y"]=$pdf_unity == "cm" ?$cover_data->{"site_cover"}:$site_cover->{"title_y"}*10;
		
		
		$cover_info["subtitle_x"]=$pdf_unity == "cm" ? $site_cover->{"subtitle_x"}:$site_cover->{"subtitle_x"}*10;
		$cover_info["subtitle_y"]=$pdf_unity == "cm" ? $site_cover->{"subtitle_y"}:$site_cover->{"subtitle_y"}*10;
		
		$cover_info["edition_x"]=$pdf_unity == "cm" ?$site_cover->{"edition_x"}:$site_cover->{"edition_x"}*10;
		$cover_info["edition_x"]=$cover_info["edition_x"]+$margin_left;
		$cover_info["edition_y"]=$pdf_unity == "cm" ?$site_cover->{"edition_y"}:$site_cover->{"edition_y"}*10;

		
		$cover_info["logo_x"]=$pdf_unity == "cm" ? $cover_data->{"logo_x"} : $cover_data->{"logo_x"}*10;
		$cover_info["logo_y"]=$pdf_unity == "cm" ? $cover_data->{"logo_y"} : $cover_data->{"logo_y"}*10;

		$cover_info["logo_width"]=$pdf_unity == "cm" ? $cover_data->{"logo_width"}:$cover_data->{"logo_width"}*10;
		$cover_info["logo_height"]=$pdf_unity == "cm" ? $cover_data->{"logo_height"}:$cover_data->{"logo_height"}*10;
		
		$cover_info["logo2_x"]=$pdf_unity == "cm" ? $site_cover->{"logo2_x"} : $site_cover->{"logo2_x"}*10;
		$cover_info["logo2_y"]=$pdf_unity == "cm" ? $site_cover->{"logo2_y"} : $site_cover->{"logo2_y"}*10;

		$cover_info["logo2_width"]=$pdf_unity == "cm" ? $site_cover->{"logo2_width"}:$site_cover->{"logo2_width"}*10;
		$cover_info["logo2_height"]=$pdf_unity == "cm" ? $site_cover->{"logo2_height"}:$site_cover->{"logo2_height"}*10;

		return $cover_info;
	}

	function getCleanArrayByOid($obj) {
		if(!$obj)
			return null;

		$a=array();
		foreach ($obj as $k => $v)
		{
			// if($k != 'undefined')
				$a[$k] = $v;
		}
		return $a;
	}

	function getOfferDoctors($data)
	{
		$doctors_db_list=$this->getDoctorsDB();
		$doctors=array();
		$offerDoctors=$this->getCleanArrayByOid($data->{"praticiens"});
		$doctorsInfos=$this->getCleanArrayByOid($data->{"praticiens_infos"});

		if($offerDoctors == "" || $offerDoctors == null)
				return null;

		if($doctors_db_list == null)
			return null;
		
			
		foreach ($doctors_db_list as $oid_doc =>$doctor)
		{
			if(isset($doctor) && $offerDoctors[$oid_doc])
			{
				// if(strtolower($doctor->last_name) =='barrage')	DebugBreak();

				$doctor_obj = new DOCTOR($doctor->oid,$doctor->civility,trim($doctor->first_name),trim($doctor->last_name),$doctor->phone);
				
				if($doctorsInfos && $doctorsInfos[$oid_doc]) 
				{
					$infos = $doctorsInfos[$oid_doc];
					if(isset($infos->phone) && $infos->phone != "")
						$doctor_obj->phone=str_replace("."," ",$infos->phone);
					if(isset($infos->email_mobile) && $infos->email_mobile != "")
						$doctor_obj->email_mobile=str_replace("."," ",$infos->email_mobile);
				}

				$doctors[$doctor->first_name.' '.$doctor->last_name]=$doctor_obj;
			}
		}
		
		usort($doctors, array("DOCTOR", "compare"));

		return $doctors;
	}

	function getOfferDoctors_old($data)
	{
		$doctors_db_list=$this->getDoctorsDB();
		$doctors=array();
		$offerDoctors=$data->{"praticiens"};
		$doctorsInfos=$data->{"praticiens_infos"};

		if($offerDoctors == "" || $offerDoctors == null)
				return null;

		if($doctors_db_list == null)
			return null;
		

		foreach ($offerDoctors as $oid => $value)
		{
			if($oid == null || $oid == "undefined" || $value != TRUE)
				continue;
			foreach ($doctors_db_list as $oid_doc =>$doctor)
			{
				if(isset($doctor) && $oid_doc == $oid)
				{
					$doctor_obj=new DOCTOR ($doctor->oid,$doctor->civility,trim($doctor->first_name),trim($doctor->last_name),$doctor->phone);
					
					if($doctorsInfos){
						foreach ($doctorsInfos as $oid_info => $infos)
						{
							if($oid == $oid_info) 
							{
								if(isset($infos->phone) && $infos->phone != "")
									$doctor_obj->phone=str_replace("."," ",$infos->phone);
								if(isset($infos->email_mobile) && $infos->email_mobile != "")
									$doctor_obj->email_mobile=str_replace("."," ",$infos->email_mobile);
							} 
						}
					}
					$doctors[$doctor->last_name]=$doctor_obj;
				}
			}
        }
		//get info
		/*foreach ($doctorsInfos as $oid => $infos)
		{
			if(isset($infos) && array_key_exists($oid, $doctors)) 
			{
				if(isset($infos->phone) && $infos->phone != "")
					$doctors[$oid]->phone=str_replace("."," ",$phone->phone);
				if(isset($infos->email_mobile) && $infos->email_mobile != "")
					$doctors[$oid]->email_mobile=str_replace("."," ",$infos->email_mobile);
			} 
			
        }*/
		
		usort($doctors, array("DOCTOR", "compare"));

		return $doctors;
	}

	var $doctors_list = null;
	function getDoctorsDB()
	{
		if($this->doctors_list !== null)
			return $this->doctors_list;

		$doctors_list[]=array();
		$url =$this->getParam('doctors_json_url');
		if(isset($_REQUEST['soid']))
			$url=$url."&entity=".$_REQUEST['soid'];
		$response=$this->getCurlRes($url,"Doctors");
		if(!$response)
			return null;
		$data = json_decode($response);
		if ($data === null && json_last_error() !== JSON_ERROR_NONE) 
			return null;

		$data=$data->{'data'};
		$doctors=$data->{'items'};
		foreach ($doctors as $doctor)
		{
			$doctor_obj=new DOCTOR(
				$doctor->{"oid"},
				$doctor->{"civility"}->value,$doctor->{"first_name"},$doctor->{"last_name"},
				$doctor->{"medical_secretary"}
			);	
			
			$doctors_list[$doctor->{"oid"}]=$doctor_obj;
        }
		return $this->doctors_list = $doctors_list;
	}

	function getSections($page,$page_data)
	{
		$sections=array();
		$parts=$page->{"parts"};
		if($page_data->{"prise_en_charge"} == true)
		{
			$sub_sections_json=$parts->{"prise_en_charge"};
			$sub_sections=$this->getSubSections($sub_sections_json);
			if($sub_sections != null && !empty($sub_sections))
			{
				$sections[]=new SECTION("Pathologies / Prises en charge",$sub_sections);
			}
		}
		if($page_data->{"pathologie"} == true)
		{
			$sub_sections_json=$parts->{"pathologie"};
			$sub_sections=$this->getSubSections($sub_sections_json);
			if($sub_sections != null && !empty($sub_sections))
			{
				$sections[]=new SECTION("Pathologies / Prises en charge",$sub_sections);
			}
		}
		if($page_data->{"plateau_technique"} == true)
		{
			$sub_sections_json=$parts->{"plateau_technique"};
			$sub_sections=$this->getSubSections($sub_sections_json);
			if($sub_sections != null && !empty($sub_sections))
			{
				$sections[]=new SECTION("Plateau Technique",$sub_sections);
			}
		}
		if($page_data->{"a_savoir"} == true)
		{
			$sub_sections_json=$parts->{"a_savoir"};
			$sub_sections=$this->getSubSections($sub_sections_json);
			if($sub_sections != null && !empty($sub_sections))
			{
				$sections[]=new SECTION("A savoir",$sub_sections);
			}
		}

		$sections[]=new Section ("Joindre un Praticien",array());
		 
		return $sections;
	}

	function getSubSections($sub_sections_json)
	{
		$sub_sections=array();
		foreach($sub_sections_json as $sub_section)
		{
			$items=$sub_section->{"items"};
			$i=0;
			foreach($items as $item)
			{
				if($item->{"text"} != "" && $item->{"display"})
				{
					$i++;
				}
			}
			if($i>0 || (trim($sub_section->title) && $sub_section->with_title))
				$sub_sections[]=$sub_section;
				
		}
		return $sub_sections;
	}
	
}

class LIVRET_PDF extends FPDI
{
	
	var $offer;
	var $page_type;
	var $coverInfos = array();
	var $summaryInfos = array();
	var $emergencyInfos= array();
	var $myLinks = array();
	var $establishment ;
	var $poleInfos=array();
	var $offersList = array();


	var $marginLeft=10;
	var $marginRight=10;
	var $marginTop=5;
	var $marginBottom=5;
	var $marginImp=5;
	var $autoPageBreak=15;
	var $unit;
	var $cover_font;

	var $header_title;
	var $header_font;
	var $header_size;
	var $footer_font;
	var $display_footer=true;

	var $title_font;
	var $title_size;
	var $subtitle_font;
	var $subtitle_size;

	var $sectiontitle_font;
	var $subsectiontitle_font;

	var $page_font;
	var $title;
	var $subtitle;

	var $num_pages=array(
			"cover"=>"1",
			"summary"=>"2",
			"emergency"=>"3",
			"pole"=>"4",
			"index"=>"5",
	);

	var $colorModel="CMYK";

	var $colors = array(
		'black' => array(0,0,0,100),
		'green_title' => array(),
		'gray' => array(),
		'blue_title' => array(100,33,0,29),
		'blue_bg' => array(),
	);

	function setTextColorByName($name) {
		$c = $this->colors[$name];
		$this->SetTextColor($c[0],$c[1],$c[2],$c[3]);
	}
	function setFillColorByName($name) {
		$c = $this->colors[$name];
		$this->SetFillColor($c[0],$c[1],$c[2],$c[3]);
	}
	function setDrawColorByName($name) {
		$c = $this->colors[$name];
		$this->SetDrawColor($c[0],$c[1],$c[2],$c[3]);
	}

	function init()
	{
		if(true)
		{
			// do not compress content (easier to debug..)
			$this->SetCompression(false);
		}
		
		//sizes
		$this->header_size='8';
		$this->title_size='20';
		$this->subtitle_size='20';

		$this->unit="mm";
		$this->SetLeftMargin($this->marginLeft);
		$this->SetRightMargin($this->marginRight);

		$this->marginTop=$this->marginTop+$this->marginImp;
		$this->SetTopMargin($marginTop);

		
		//fonts
		$this->setElementFont('cover_font','HelveticaNeueLTStd-Th');
		$this->setElementFont('header_font','HelveticaNeueLTStd-Th');
		$this->setElementFont('footer_font','HelveticaNeueLTStd-Md');
		$this->setElementFont('title_font','HelveticaNeueLTStd-Md');
		$this->setElementFont('subtitle_font','HelveticaNeueLTStd-Th');
		$this->setElementFont('page_font','HelveticaNeueLTStd-Th');
		$this->setElementFont('bullet_font','HelveticaNeueLTStd-Md');
		$this->setElementFont('sectiontitle_font','HelveticaNeueLTStd-Md');
		$this->setElementFont('subsectiontitle_font','HelveticaNeueLTStd-Md');

		$this->setElementFont('other_font','helveticab');

		$this->SetAutoPageBreak(true,$this->autoPageBreak-$this->marginImp);  //saut de page
	}

	function generatePDFPage($type)
	{
		if($type == "cover")
		{
			$this->page_type="cover";
			$this->generateCoverPage();
		}
		if($type == "summary")
		{
			$this->page_type="summary";
			$this->generateSummaryPage();
		}
		if($type == "emergency")
		{
			$this->page_type="emergency";
			$this->generateEmergencyPDF();
		}
		if($type == "index")
		{
			$this->page_type="index";
			$this->generateIndex();
		}
		if($type == "offer")
		{
			$this->page_type="offer";
			$this->generateOfferPDF();
		}
		if($type == "pole" || $type == "pôle" )
		{
			$this->page_type="pole";
			$this->generatePolePDF();
		}
		if($type == "note")
		{
			$this->page_type="note";
			$this->generateNotePDF();
		}
		if($type == "last_cover")
		{
			$this->page_type="last_cover";
			$this->generateCoverPage();
		}
		
	}

	function displayLogo($logo,$logo_x,$logo_y,$logo_h)
	{
		//logo
		$logo=str_replace("/afiles",NX_CLIENT_ROOT."files",$logo);
		$f=fopen($logo ,'rb');
		if($f)
		{
			$wPdf = $this->wPt;
			$hPdf = $this->hPt; 

			$size = getimagesize($logo);
			$wImg = $size[0];
			$hImg = $size[1];

			$lMargin = $this->lMargin;
			$rMargin = $this->rMargin;

			if($this->unit == "mm")
			{
				//points to mm
				$wPdf=0.352778*$wPdf;
			}
			if($this->unit == "cm")
			{
				//points to cm
				$wPdf=0.0352778*$wPdf;
			}

			// $logo_h = $cover_info["logo_height"];
			$logo_w = $this->getImageWidth($wImg,$hImg,$logo_h);

			$wPdf=$wPdf-$lMargin;

			$logo_y=$logo_y+$this->marginImp;
			if($logo_x <0)
			{
				$logo_x	=$logo_x-$logo_w;
				$logo_x=$wPdf+$logo_x;
				$logo_x=$logo_x+$this->marginImp;
			}
			else
				$logo_x	=$logo_x+$this->marginImp;

			//$this->Cell($logo_x);
			$this->Image($logo,$logo_x,$logo_y,"",$logo_h); 
		}
	}

	// ============== PAGE : COUVERTURE =============================

	function generateCoverPage()
	{
		if($this->page_type == "cover")
		{
			if($this->coverInfos == null && empty($this->coverInfos) )
			{
				return;
			}
			$cover_info=$this->coverInfos;
			$this->setElementFont('cover_font','HelveticaNeueLTStd-Th');
			$this->AddPage();

			//image de couverture
			$img = $cover_info["image"];
			$img=str_replace("/afiles",NX_CLIENT_ROOT."files",$img);
			$f=fopen($img ,'rb');
			if($f)
			{
				$bg_x=$cover_info["bg_x"];
				$bg_y=$cover_info["bg_y"];
				if($this->unit == "mm")
				{
					$bg_x=$bg_x*10;
					$bg_y=$bg_y*10;
				}
				$this->Image($img,$bg_x+2,$bg_y,115-2-2,120);
			}

			//logo
			$this->displayLogo($cover_info["logo"],$logo_x = $cover_info["logo_x"],$logo_y = $cover_info["logo_y"],$cover_info["logo_height"]);
/*
			//logo groupe
			$this->displayLogo($cover_info["logo2"],$logo_x = $cover_info["logo2_x"],$logo_y = $cover_info["logo2_y"],$cover_info["logo2_height"]);
*/
			$title =$cover_info["title"];
			$subtitle =$cover_info["subtitle"];
			$titre_edition =$cover_info["titre_edition"];

			
			//titre
			$this->SetFont($this->cover_font,'',21);
			$this->setMyColText('BLUE');
			$x=$cover_info["title_x"];
			$y=$cover_info["title_y"];
			$this->SetY($y);
			//$this->Cell(0,12,$title,0,0,'C');
			// $this->MultiCell(0,8,$title,0,'C');
			$currentY=$this->GetY();
			$this->Ln(0);

			//sous titre
			$this->SetFont($this->cover_font,'',15);
			$x=$cover_info["subtitle_x"];
			$y=$cover_info["subtitle_y"];
			$diff=$currentY-$y;
			if($diff>0)
				$y=$y+$diff;
			$this->SetY($y);
			$this->Cell(0,12,$subtitle,0,0,'C');
			//$this->Cell(0,12,$y,0,0,'C');
			$this->Ln(0);
			

			//edition
			$x=$cover_info["edition_x"];
			$y=$cover_info["edition_y"];
			$this->SetXY($x+$this->marginLeft,$y);
			$this->SetFont($this->cover_font,'',12);
			$this->setMyColText();
			$this->MultiCell(0,10,$titre_edition,"",3);
		}else
		{
			$current_page=$this->PageNo();
			$next_page=$current_page+1;
			while( ($next_page % 4) != 0)
			{
				$this->generatePDFPage("note");
				$next_page=$this->PageNo()+1;
				$this->display_footer=false;
			}

			$this->display_footer=false;
			$this->AddPage();
			//$this->SetMargins(10,0,10);

			$this->Ln(22);
			$this->SetFont($this->title_font,'',11);
			$this->SetTextColor();
			$text="Vous  voulez recevoir des informations ? ";
			$text=html_entity_decode($text, ENT_QUOTES | ENT_HTML5);
			$text=iconv('UTF-8', 'windows-1252', stripslashes($text));
			$this->MultiCell(0,6,$text,"",'C','');
			$text="Connaître les événements organisés par les praticiens ? ";
			$text=iconv('UTF-8', 'windows-1252', stripslashes($text));
			$this->Cell(0,6,$text,0,1,'C');
			$text="Etre informés des nouveautés au sein de l’établissement? (Spécialités, services, matériel ?...)";
			$text=iconv('UTF-8', 'windows-1252', stripslashes($text));
			$this->MultiCell(0,6,$text,"",'C','');


			$this->Ln(25);
			$text="Laissez-nous votre adresse e-mail sur :";
			$text=html_entity_decode($text, ENT_QUOTES | ENT_HTML5);
			$text=iconv('UTF-8', 'windows-1252', stripslashes($text));
			$this->MultiCell(0,3,$text,"",'C','');
			$this->Ln(2);
			//$this->setTextColorByName("blue_title");
			$this->SetTextColor(89,23,0,8);
			$text="http://praticien.ramsaygds.fr/liens";
			$text=html_entity_decode($text, ENT_QUOTES | ENT_HTML5);
			$text=iconv('UTF-8', 'windows-1252', stripslashes($text));
			$this->MultiCell(0,3,$text,"",'C','');

			$img = NX_DOC_ROOT."/nx/skins/lightsky/images/livret_fpdf/cursor.jpg";
			$f=fopen($img ,'rb');
			if($f)
			{
				$x=$this->w-$this->marginLeft-$this->marginRight-($this->marginImp);
				$this->Image($img,$x/2,$this->GetY(),15,15);
				
			}

			$text=utf8_decode("est un établissement Ramsay Santé");
			$establishment=$this->establishment;

			if(preg_match("#^[aeiouyhAEIOUYH]#", $establishment))
			{
			   $establishment="L' ".$establishment;
			}
			else
			{
			   $establishment="La ".$establishment;
			}


			$this->setY(118);
			$this->SetTextColor();
			$this->SetFont($this->page_font,'',7);
			//$this->Cell(10);
			$text=$establishment." ".$text;
			$this->MultiCell(0,4,$text,"",'C','');
			$this->Ln(3);
			$this->SetFont($this->title_font,'',7);
			$this->MultiCell(0,4,utf8_decode("A propos de Ramsay Santé "),"",'L','');
			$this->Ln(1);
			$this->SetFont($this->page_font,'',7);

			$cover_info=$this->coverInfos;

			// back_cover_text
			if($cover_info["back_cover_text"])
			{
				$text = $cover_info["back_cover_text"];
			}
			else
			{
			$text="Premier groupe privé de soins et services à la santé, Ramsay Santé compte 22 000 salariés dans 124 établissements et centres, et travaille avec 6 000 praticiens, qui représentent la première communauté libérale de France. Acteur majeur de l’hospitalisation, Ramsay Générale de Santé couvre l'ensemble de la chaîne de soins, dans trois métiers : médecine-chirurgie-obstétrique, soins de suite et de réadaptation et santé mentale. Ramsay Générale de Santé développe une offre de soins originale associant qualité, sécurité de la prise en charge et efficience de l’organisation. Le groupe propose une prise en charge globale avec un accompagnement personnalisé, avant, pendant et après l’hospitalisation, qui prend en compte tous les besoins et attentes du patient ; il participe aux missions de service public de santé et au maillage sanitaire du territoire.";
			}

			$text=iconv('UTF-8', 'windows-1252', stripslashes($text));
			$this->MultiCell(0,4,$text,"",'J','');
			$this->Ln(1);
			$text="En savoir plus : www.ramsaygds.fr";
			$this->MultiCell(0,4,$text,"",'L','');
			$this->displayLogo($cover_info["logo2"],$logo_x = $this->getX(),$logo_y = $this->GetY()-5,25);
		}
	}


	// ============== PAGE : SPECIALITES MARQUANTES =============================

	function generateSummaryPage()
	{
		$this->AddPage();

		$this->ln(1);
		$this->setMyColText();
		$this->SetFont($this->page_font,'',12);

		$establishment=$this->establishment;
		if(preg_match("#^[aeiouyhAEIOUYH]#", $establishment))
		   $establishment="L' ".$establishment;
		else
		   $establishment="La ".$establishment;

		$summaryText=$establishment.utf8_decode(" appartient au pôle ");
		if($this->GetStringWidth($summaryText)>80 && $this->GetStringWidth($summaryText)<90)
		{
			$this->MultiCell(0,5,$summaryText,"",'L','');
			$this->ln(1);
			$summaryText="";
		}
		$poleName = $this->summaryInfos["pole"];
		$summaryText=$summaryText.utf8_decode("{$poleName} du premier groupe français de soins et services à la santé, Ramsay Santé.");
		$this->MultiCell(0,5,$summaryText,"",'C','');
		$this->ln(1);
		$this->SetFont('HelveticaNeueLTStd-Md','',12);
		$this->MultiCell(0,5,utf8_decode("Son activité repose sur plusieurs spécialités fortes :"),"",'L','');
		$this->ln(4);

		if($this->summaryInfos != null)
		{
			$bed_number=$this->summaryInfos["bed_number"];
			$bed_number_desc=$this->summaryInfos["bed_number_desc"];
			$nb_doctors=$this->summaryInfos["nb_doctors"];
			$offers=$this->summaryInfos["offers"];
			$and_also=$this->summaryInfos['and_also'];
		}
		else
		{
			return;
		}
		
		$currentPosition=$this->displaySummaryOffers($offers);

		// section "and also"
		if($and_also)
		{
			$currentY=$currentPosition;
			$this->SetY($currentY+23);
			$this->ln(1);
			$this->setMyColText();
			$this->SetFont($this->page_font,'',11);
			$this->MultiCell(0,5,utf8_decode("Et aussi : ".$and_also),"",'C','');
			$this->ln(1);
			$currentPosition += 20;
		}

		// autres informations
		$this->displaySummaryInfo($img_bed_numbers,$bed_number,$bed_number_desc,$img_doctors,$nb_doctors,$currentPosition);		
	}

	function displaySummaryOffers($offers)
	{
		//DebugBreak();
		$this->SetFont('HelveticaNeueLTStd-Md','',8);
		$this->setMyColText();
		
		$currentY=$this->GetY();
		if($offers == null || empty($offers))
		{
			return $currentY;
		}
	
		//Nombre total d'offres à afficher
		$nbOffers = count($offers);
		if($nbOffers == 1)
		{
			$cellWidth=96;
			$iconPosition=42;

		}elseif($nbOffers<=2)
		{
			$cellWidth=47;
			$iconPosition=16;
		}
		else
		{
			$cellWidth=32;
			$iconPosition=8;
		}

		$iconHeight=15;
		$textHeight=3;
		$countOffers=0;
		$maxOffers=9;
		for ($i=0;$nbOffers>$i;$i++)
		{
			if($countOffers>=$maxOffers)
				break;
			$offer=$offers[$i];
			$currentX=$this->GetX();
			
			//si on dépasse trois par ligne
			if($i >= 3)
			{
				$newOffers=array();
				for ($j = 0;$nbOffers>$i;$j++) 
				{
					$newOffers[$j]=$offers[$i];
					$i++;
				}
				$i=0;
				$offers=$newOffers;
				$nbOffers=count($offers);
				if($nbOffers == 1)
				{
					$cellWidth=96;
					$iconPosition=43;

				}elseif($nbOffers<=2)
				{
					$cellWidth=47;
					$iconPosition=16;
				}
				else
				{
					$cellWidth=32;
					$iconPosition=8;
				}

				$offer=$offers[$i];
				
				//saut de ligne
				$currentX=$this->marginLeft;
				$currentY=$currentY+$iconHeight+$textHeight+10;
				$this->SetXY($currentX,$currentY);
				
			}
			
			$img = $offer->iconFile;
			$img=str_replace("/afiles",NX_CLIENT_ROOT."files",$img);
			$f=fopen($img ,'rb');
			if($f)
			{
				$this->Cell($cellWidth,$iconHeight,$this->Image($img,$this->GetX()+$iconPosition,$this->GetY(),15,15),0,1,'C',false);
			}
			else
			{
				$this->MultiCell($cellWidth,$textHeight,"",0,'C',false);
			}

			$this->SetXY($currentX,$currentY+15);
			$this->MultiCell($cellWidth,$textHeight,utf8_decode($offer->summary_alias),0,'C',false);

			$currentX=$currentX+$cellWidth;
			$this->SetXY($currentX,$currentY);
			$countOffers++;
		}

		return $currentY+5;
	}


	function displaySummaryInfo($img_bed_numbers,$bed_number,$bed_number_desc,$img_doctors,$nb_doctors,$currentPosition)
	{
		$this->Ln(1);
		$this->SetFont('HelveticaNeueLTStd-Md','',8);
		$currentY=$currentPosition;
		$this->SetY($currentY+25);

		$img_bed_numbers = NX_DOC_ROOT."/nx/skins/lightsky/images/livret_fpdf/bed_numbers.jpg";
		$img_doctors = NX_DOC_ROOT."/nx/skins/lightsky/images/livret_fpdf/doctors_nb.jpg";
		$f_bed_numbers=fopen($img_bed_numbers,'rb');
		$f_doctors=fopen($img_doctors  ,'rb');
		if($f_bed_numbers && $f_doctors)
		{
			//$this->SetX(20);
			if($bed_number != "")
				$this->Image($img_bed_numbers,29,$this->GetY(),12,12);
			if($nb_doctors != "")
				$this->Image($img_doctors,79,$this->GetY(),12,12);
			$this->ln();
		}

		$this->SetY($this->GetY()+9);
		$this->SetTextColor(89,23,0,8);
		$this->SetFontSize(22);
		
		if($bed_number != "")
			$this->Cell(50,8,$bed_number,0,"","C");
		if($nb_doctors != "")
			$this->Cell(50,8,$nb_doctors,0,"","C");
		$this->ln();

		
		$this->SetFontSize(8);
		$this->SetTextColor(89,23,0,8);
		if($bed_number != "")
		{
			$text=($bed_number_desc != "") ? "lits et places dont" : "lits et places";
			$this->Cell(50,3,$text,'',"","C");
		}
		if($nb_doctors != "")
			$this->Cell(50,3,"praticiens",'',"","C");
		$this->ln();

		$this->setMyColText();
		if($bed_number != "")
			$this->MultiCell(50,3,$bed_number_desc,0,"C");
		$this->ln();
	}


	// ============== PAGE : SOMMAIRE/INDEX =============================

	function indexAddPage()
	{
		$this->AddPage();
		//header
		//surimpression
		$this->SetTextColor(14,5,0,100);
		$this->SetDrawColor(14,5,0,100);
		$this->SetFont($this->header_font,'',$this->header_size);
		$this->setY($this->marginTop);
		$this->Cell(0,4,$this->header_title,0,0,'C');
		$this->Ln(4);
		$this->SetLineWidth(1);
		$this->Cell(0,3,"","B",0,'');
		$this->Ln(5);

		$this->SetTextColor(43,14,0,8);

		/*
		$this->SetFont($this->title_font,'',43);
		$this->RotatedText(20,200,'Index des offres de soins ',90);
		*/
		$this->writeVerticalTitle('Index des offres de soins');

		$this->ln(6);
	}

	function generateIndex()
	{
		$offers_list=$this->offersList;
		$this->indexAddPage();
		//DebugBreak();
		$totalOffers=0;
		foreach ($offers_list as $offer_page)
		{
			if($offer_page->display == true)
			{
				$totalOffers++;
			}
		}
		//numerotation
		if($totalOffers >= 44)
			$numPage=$this->PageNo()+2;
		else
			$numPage=$this->PageNo()+1;
		foreach ($offers_list as $offer_page)
		{
			if($offer_page->display == true)
			{
				$this->SetFont($this->page_font,'',8);
				$this->SetTextColor(43,14,0,8);
				$this->Cell(13);
				$this->Cell(3,2,"->", 0, 0, 'L', false );
 
				//le titre de la page
				$titrePage = iconv('UTF-8', 'windows-1252', stripslashes($offer_page->title));
				 
				$link= $this->addLink();
				//on créé un lien
				$this->myLinks[$numPage] = $link;
				 
				// on écrit le nom de la page et on y associe le lien
				// surinpression
				$this->SetTextColor(14,5,0,100);
				$this->cell(75,2,$titrePage,0,0,'L',false,$this->myLinks[$numPage]);
				//on écrit le numéro de la page
				if($numPage <=9)
					$this->cell(4,2,"0".$numPage,0,2,'R',false);
				else
					$this->cell(4,2,$numPage,0,2,'R',false);
				$this->ln();
				$numPage++;
				if($numPage == 51)
				{
					$this->indexAddPage();
				}
			}
		}

	}

	// ============== PAGE : URGENCES =============================

	function generateEmergencyPDF()
	{
		$urgences=$this->emergencyInfos["urgencies"];
		$praticiens=$this->emergencyInfos["praticiens"];
		$data = array();
		if($this->emergencyInfos["passages"] != "" && $this->emergencyInfos["passages_text"] != "")
			$data[]=array("number" => $this->emergencyInfos["passages"], "text" => $this->utf8decodeAndSlashes($this->emergencyInfos["passages_text"]));
		if($this->emergencyInfos["nb_beds"] != "" && $this->emergencyInfos["nb_bed_text"] != "")
			$data[]=array("number" => $this->emergencyInfos["nb_beds"], "text" => $this->utf8decodeAndSlashes($this->emergencyInfos["nb_bed_text"]));
		if($this->emergencyInfos["boxes"] != "" && $this->emergencyInfos["box_text"] != "")
			$data[]=array("number" => $this->emergencyInfos["boxes"], "text" => iconv('UTF-8', 'windows-1252', stripslashes($this->emergencyInfos["box_text"])));

		$this->AddPage();

		//surimpression
		$this->writeHeader_emergency();

		// titre vertical
		$title=utf8_decode($this->emergencyInfos["page_title"]);
		$this->SetTextColor(43,14,0,8);

		$this->writeVerticalTitle($title);

		$margin=27;
		if(count($urgences) != 0)
		{
			foreach($urgences as $urgence)
			{
				$this->Ln(2);
				$this->SetX($margin);
				$this->SetFont($this->page_font,'',10);
				$this->SetTextColor(89,23,0,8);;
				$y=$this->GetY();
				if($urgence["title"])
					$this->MultiCell(0,4,"->",'0','L','');

				$this->SetXY($margin+4,$y);
				$this->MultiCell(0,4,$urgence["title"],'0','L','');

				if($urgence["display_description"])
				{
					if($urgence["description"] != "")
					{
						$this->SetTextColor(14,5,0,100);
						$this->SetX($margin+4);
						$this->SetFont($this->page_font,'',10);
						$this->MultiCell(0,6,$urgence["description"],'0','L','');
					}
					else
						$this->MultiCell(0,6,' ','0','L','');
				}			
				
				foreach ($urgence["contacts"] as $contact)
				{
					if($contact->{'label'})
					{
						$this->SetX($margin+4);
						$this->SetFont($this->page_font,'',8);
						//surimpression
						$this->SetTextColor(14,5,0,100);
						$this->Cell(30,4,utf8_decode($contact->{'label'}." : "),0,0,'L');
						$this->SetFont($this->title_font,'',8);
						$this->Cell(40,4,utf8_decode($contact->{'text'}),'0','1','L');
					}
				}

			}
			$this->Ln(2);
			$this->drawLine($margin+$this->marginImp,$this->GetY(),100);
		}


		if(count($data) != 0)
		{
			$this->Ln(2);
			$this->SetTextColor(85,22,0,11);
			foreach ($data as $element)
			{
				$this->SetX($margin+$this->marginImp);
				$this->SetFont($this->title_font,'',30);
				$w=$this->GetStringWidth($element["number"]);

				$this->Cell($w+3,10,$element["number"],0,0,'L');
				$this->SetFont($this->title_font,'',8);
				$this->SetXY($margin+$this->marginImp+$w+2,$this->GetY()+4);
				$text=$element["text"];
				//$text=utf8_decode($text);
				//$text=html_entity_decode($text, ENT_QUOTES | ENT_HTML5);
				//$text=iconv('UTF-8', 'windows-1252',$text);
				//$text=iconv('UTF-8', 'windows-1252', stripslashes($text));
				//$text=htmlentities($text, ENT_QUOTES, "UTF-8");
				$modif = array("Ã©","Ã","Ã´","Ã¢","Ã®","Ã¨","Ãª");
				$amodif = array("é","è","&ocirc;","&acirc;","&icirc;","&egrave;","&ecirc;");

				$text = str_replace($modif,$amodif,$text);
				$this->Multicell(0,4,$text,0,1,'L');
				$this->Ln(2);
			}
			$this->drawLine($margin+$this->marginImp,$this->GetY(),100);
		}		
		
		if(count($praticiens) != 0)
		{
			$this->Ln(2);
			$this->SetFont($this->title_font,'',12);
			$this->SetTextColor(0,0,0,0);
			$text="Joindre un Praticien";
			$img = NX_DOC_ROOT."nx/skins/lightsky/images/livret_fpdf/phone-volume.jpg";
			$f=fopen($img ,'rb');
			if($f)
			{
				$this->Image($img,$margin+$this->marginImp,$this->GetY(),6,6);
				$this->SetXY($margin+$this->marginImp+6,$this->GetY());
				$this->MultiCell(0,5,utf8_decode($text),"",3);
			}
			else
			{
				$this->SetX($margin+$this->marginImp);
				$this->MultiCell(0,5,utf8_decode($text),"",3);
			}

			$this->Ln(2);
			foreach ($praticiens as $doctor)
			{
				$civility=$doctor->civility;
				if($civility == '' || $civility == 'DR' )
					$civility='Dr';

				$doctor_name=$civility." ".$this->formatName($doctor->first_name)." ".strToUpper($doctor->last_name);
				// $doctor_name="Dr ".$this->formatName($doctor->first_name)." ".strToUpper($doctor->last_name);

				$doctor_name = iconv('UTF-8', 'windows-1252', stripslashes($doctor_name));
				$doctor_cord="";
				if($doctor->phone != "")
				{
					$doctor_cord=" ".$doctor->phone;
				}
				/*if($doctor->phone2 != "")
				{
					$doctor_cord=$doctor_cord." ".$doctor->phone2;
				}*/

				$this->SetX($margin+$this->marginImp+5);
				$this->SetFont($this->page_font,'',8);
				//surimpression
				$this->SetTextColor(14,5,0,100);
				$this->Cell(40,5,$doctor_name,0,0,'L');
				if($doctor_cord != "")
					$this->Cell(1,5,":",0,0,'L');
				$this->Cell(30,4,$doctor_cord,0,1,'L');

			}
			$this->Ln(2);
			$this->drawLine($margin+$this->marginImp,$this->GetY(),100);
		}

	}

	// ============== PAGE : POLE =============================

	function setPoleTitle()
	{
		$this->Ln(2);
		$this->SetFont($this->title_font,'',18);
		$this->SetTextColor(100,29,0,11);
		$this->MultiCell(0,10,utf8_decode("Le Pôle"),'0','L','');
		$this->setMyColText();
		$this->Ln(1);
		$cellWith=115-$this->margin100Left-$this->marginRight-($this->marginImp*2);
		
		$this->SetFont($this->subtitle_font,'',16);
		$this->MultiCell(50,9,utf8_decode($this->poleInfos["pole_name"]),"0","L",false);
		$this->SetDrawColor(0,0,0,100);
		$this->SetLineWidth(1);
		$this->Cell(0,2,"","B",0,'');

		$img=$this->poleInfos["map_file"];
		$img=str_replace("/afiles",NX_CLIENT_ROOT."files",$img);
		$f=fopen($img ,'rb');
		if($f)
		{
			$img_size = getimagesize($img);
			$W_Src = $img_size[0]; // largeur source
			$H_Src = $img_size[1]; // hauteur source
			//if($W_Src>60)

			$this->Image($img,42+$this->marginLeft+$this->marginImp,20,50);
			//$this->Image($img,50+$this->marginLeft+$this->marginImp,20,60);
		}		
	}

	function sortOtherSites($sites) 
	{
		$sites2 = array();

		// remove first site
		// array_shift($sites);

		usort($sites, function($a, $b)
		{
			return strcmp($a->data->_index, $b->data->_index);
		});

		return $sites;
	}

	function generatePolePDF()
	{
		$this->AddPage();
		//nombre etablissements et lits
		$this->setPoleTitle();
		//$y_pos =76;

		$y_pos =73;

		$offsetY = 0+$this->poleInfos["offsetY"];
		if($offsetY)
			$y_pos += $offsetY*10; 

		$x_pos = $this->GetX();
		$this->SetY($y_pos);
		$img_pole = NX_DOC_ROOT."/nx/skins/lightsky/images/livret_fpdf/pole_img.jpg";
		if(fopen($img_pole,'rb'))
		{
			$this->SetTextColor(0,0,0,0);
			$this->SetXY($x_pos,$y_pos);
			$this->Image($img_pole,$this->marginLeft+1,$y_pos+6,95,20);
			
			$this->SetXY(25,$y_pos+15);
			$x = $this->GetX();
			$this->SetFont('HelveticaNeueLTStd-Md','',20);
			$nb_hp=$this->poleInfos["nb_hp"];
			$this->Cell(5,5,$nb_hp,0,0,"L",false);
			$this->SetFont("HelveticaNeueLTStd-Th",'',9);
			if($nb_hp > 1)
				$nb_hp_txt="établissements";
			else
				$nb_hp_txt="établissement";
			$this->Cell(32,5,utf8_decode($nb_hp_txt),0,0,"L",false);
			
			$x = $this->GetX()+5;
			$this->SetXY($x,$y_pos+17);
			$this->SetFont('HelveticaNeueLTStd-Md','',20);
			$wTexte=$this->GetStringWidth($this->poleInfos["nb_beds"]);
			$this->Cell($wTexte+1,5,$this->poleInfos["nb_beds"],0,0,"L",false);
			$this->SetXY($this->GetX(),$y_pos+17);
			$this->SetFont("HelveticaNeueLTStd-Th",'',9);
			$this->Cell(15,5,"lits et places",0,0,"L",false);
		}

		$y_pos=$y_pos+30;
		$this->SetXY($this->marginLeft,$y_pos);
		$this->setMyColText();
		$this->SetFont('HelveticaNeueLTStd-Md','',9);
		$description= $this->poleInfos["description"];
		if($description != "")
			$this->MultiCell(0,4,$description,0,'J',false);

		$y_pos=$this->GetY();
		$this->SetY($y_pos);
		$sites= $this->poleInfos["sites"];

		$this->Ln(1);
		$i=1; //numerotation
		$add_new_page=true; //saut de page pour centre d'imagerie
		//***sites
		//afficher le site courant en premier
        $otherSites = array();
		foreach($sites as $site)
		{
			$name=iconv('UTF-8', 'windows-1252', stripslashes($site->name));
            if($site->selected == TRUE )
            {
			    if($site->type == "Etablissement" && $name == $this->establishment)
			    {
				    $this->displayPoleSite($site,$i);
				    $i++;
			    }
                else
                {
                    array_push($otherSites,$site);
                }
            }
			// if($i != 1) break;
		}
		//afficher les autres sites
		$espaces=array();
		
		// ordre par index
		$sites = $this->sortOtherSites($otherSites);

        $i = 2;

		foreach($sites as $site)
		{
			$name=iconv('UTF-8', 'windows-1252', stripslashes($site->name));

			// page break requested by user after this site
			if($site->data && $site->data->pageBreak)
				$pageBreak = true;
			else
				$pageBreak = false;
			
			if($site->type == "Etablissement" && $site->selected == TRUE && $name != $this->establishment)
			{
				if(strstr($name, 'espace') || strstr($name, 'Espace')) {
					$espaces[]=$site;
					continue;
				} 

				if($add_new_page && ($i == 7 && $offsetY>=0)){
					$add_new_page=false;
					$this->AddPage();
				}
				
				$this->displayPoleSite($site,$i);
				
				if($add_new_page && $pageBreak) 
				{
					$add_new_page=false;
					$this->AddPage();
				}

				$i++;
			}
		}
		
		//centres d'imageries
		$count=0; 
		foreach($sites as $site)
		{
			if($site->type == "Centre Imagerie" && $site->selected == TRUE)
			{
				$count++;
			}
		}
		if($count >0)
		{
			$this->setMyColText();
			$this->SetFont('HelveticaNeueLTStd-Th','',20);
			if(count($sites) >=4 )
			{
				if($add_new_page){
					$add_new_page=false;
					$this->AddPage();
					$this->Ln(6);
				}
				else{
					$this->Ln(5);
				}
					
			}

			$title=iconv('UTF-8', 'windows-1252', stripslashes("Sociétés d’Imagerie Médicale"));
			$this->MultiCell(0,4,$title,0,'L',false);
			$this->Ln(3);
			$i=1; //numerotation
			foreach($sites as $site)
			{
				if($site->type == "Centre Imagerie" && $site->selected == TRUE)
				{
					$this->displayPoleSite($site,$i);
					$i++;
				}
			}
		}
		
		//espaces
		$nbr_espaces=count($espaces);
		if($nbr_espaces >0)
		{
			$this->setMyColText();
			$this->SetFont('HelveticaNeueLTStd-Th','',20);
			if(count($sites) >=4 )
			{
				if($add_new_page){
					
					$this->AddPage();
					$this->Ln(6);
				}
				else{
					$this->Ln(5);
				}
					
			}

			$title=iconv('UTF-8', 'windows-1252', stripslashes("Etablissement Partenaire"));
			$this->MultiCell(0,4,$title,0,'L',false);
			$this->Ln(3);
			$i=1; //numerotation
			foreach($espaces as $espace)
			{
				if($espace->selected == TRUE)
				{
					if($i == 7){
						$this->AddPage();
						$add_new_page=false;
					}
					
					$this->displayPoleSite($espace,$i);
					$i++;
				}
				
				
			}
		}

		$this->SetDrawColor(0,0,0,0);
	}

	function displayPoleSite($site,$number)
	{
		$this->Ln(3);
		$this->SetFont($this->title_font,'',8);
		$this->SetTextColor(0,0,0,0);
		$x=$this->GetX();
		$color=$site->color;
		if($color != '')
		{
			$c=explode(',',$color);
			$this->SetFillColor($c[0],$c[1],$c[2],$c[3]);
		}
		else
			$this->SetFillColor(100,29,0,11);

		$this->Circle($x+2.5,$this->GetY()+2.5,2.5,'F');
		$this->SetX($x);
		$this->Cell(5,5,utf8_decode($number),0,0,'C','');
		$this->SetFont($this->title_font,'',14);
		$this->SetTextColor(100,29,0,11);
		
		$name=iconv('UTF-8', 'windows-1252',html_entity_decode(stripslashes($site->name), ENT_QUOTES | ENT_HTML5));
		$name=str_replace ("imagerie","Imagerie",$name);
		$string1=iconv('UTF-8', 'windows-1252', stripslashes("médicale"));
		$string2=iconv('UTF-8', 'windows-1252', stripslashes("Médicale"));
		$name=str_replace ($string1,$string2,$name);
		$this->MultiCell(0,5,$name,'0','L','');
		$this->Ln(2);

		$this->setMyColText();
		$this->SetFont("HelveticaNeueLTStd-Th",'',8);
		/*if($site->{address_line1} != null && $site->{address_line1} != "")
			$this->MultiCell(0,3,utf8_decode($site->{address_line1})." ".utf8_decode($site->{cp})." ".utf8_decode($site->{city}),'0','L','');*/

		$address="";
		if($site->address_line1 != null && $site->address_line1 != "")
			$address=$site->address_line1;
		if($site->cp != "")
			$address=$address." ".$site->cp;
		if($site->city != "")
			$address=$address." ".$site->city;
		if($address != "")
			$this->MultiCell(0,3,utf8_decode(trim($address)),'0','L','');

		if($site->phone != "" && $site->phone != null)
		{
			$this->Ln(1);
			$x=$this->GetX();
			$y=$this->GetY();
			$this->SetFillColor(100,29,0,11);
			$this->Circle($this->GetX()+1.5,$this->GetY()+1.5,0.7,'F');
			$this->SetXY($x+2,$y);
			$this->Cell(0,3,utf8_decode("Tél. : ".$site->phone),0,1,'L','');
		}
		if($site->email != "" && $site->email != null)
		{
			$this->Ln(1);
			$x=$this->GetX();
			$y=$this->GetY();
			$this->SetFillColor(100,29,0,11);
			$this->Circle($this->GetX()+1.5,$this->GetY()+1.5,0.7,'F');
			$this->SetXY($x+2,$y);
			$this->Cell(0,3,utf8_decode("Email. : ".$site->email),0,1,'L','');
		}

		// site description
		if($site->data && $site->data->description)
		{
			$desc=html_entity_decode(stripslashes($site->data->description), ENT_QUOTES | ENT_HTML5);

			$this->Ln(1);
			$this->MultiCell(0,3,$desc,'0','L','');
		}
	}

	// set text colour in CMYK
	function SetTextColor_CMYK($c,$m,$y,$k) {
		$this->TextColor = sprintf('%.2F %.2F %.2F %.2F k', $c / 100, $m / 100, $y / 100, $k / 100);
		$this->ColorFlag = ($this->FillColor != $this->TextColor);
		nxError("FPDF TEXT CMYK COLOR = ($c,$m,$y,$k) ".$this->TextColor);
	}


	// ============== PAGE : NOTES =============================

	function generateNotePDF()
	{
		//$this->display_footer=false;
		$this->AddPage();

		$this->SetTextColor_CMYK(0,0,0,29);
		$this->SetDrawColor(0,0,0,100);
		$this->SetFont($this->title_font,'',"24");
		$this->MultiCell(0,12,"Notes",'0','L','');
		$this->SetLineWidth(0);
		$this->Cell(0,-1,"","B",0,'');
		//$this->Ln(1);
		$this->SetDrawColor(0,0,0,29);
		for($i=0;$i<17;$i++)
		{
			$this->SetLineWidth(0);
			$this->Cell(0,1,"","B",0,'');
			$this->Ln(10);
		}
	}

	// ============== PAGE : OFFRE DE SOIN =============================

	function generateOfferPDF()
	{
		$this->AliasNbPages();
		$this->AddPage();
	
		//content
		$this->setOfferTitle();
		$this->setSections($this->offer->getSections());
	}

	function setOfferTitle()
	{
		$this->SetFont($this->title_font,'',$this->title_size);
		$this->SetDrawColor(0,0,0,100);
		$this->setMyColText('GREEN');
		if(!$this->subtitle)
		{
			$this->MultiCell(0,10,utf8_decode($this->title),'0','L',false);
			$this->setMyColText();
			$this->SetLineWidth(0);
			$this->Cell(0,-1,"","B",0,'');
			$this->Ln(1);
		}
		else
		{
			$this->MultiCell(0,10,utf8_decode($this->title),'0','L','');
			$this->setMyColText();
			$this->SetFont($this->subtitle_font,'',$this->subtitle_size);
			$this->Ln(1);
			$this->MultiCell(0,6,utf8_decode($this->subtitle),"0","L",false);
			$this->SetLineWidth(0);
			$this->Cell(0,2,"","B",0,'');
			$this->Ln(7);
		}
		
		
	}

	function setSections($offer_sections)
	{
		$offer=$this->offer;
		foreach($offer_sections as $section)
		{
			//DebugBreak();
			$sub_sections=$section->sub_sections;
			if($sub_sections != null && $section->name != "Joindre un Praticien")
			{
				//section title
				$this->SetFont($this->sectiontitle_font,'',14);
				$this->setMyColText();
				if($section->name == "A savoir")
				{
					$this->SetFont($this->sectiontitle_font,'',12);
					$this->SetFillColor(0,0,0,29);
					$this->SetTextColor(0,0,0,0);
					$this->MultiCell(20,5,utf8_decode($section->name),"",3,'true');
				}
				else
				{
					$this->MultiCell(0,5,utf8_decode($section->name),"",3,'');
				}
				
				//sub section
				$i=0;
				foreach ($sub_sections as $sub_section)
				{
					$dispaly_section = false;
					if(trim($sub_section->{"title"}) && $sub_section->{"with_title"})
						$dispaly_section = true;
					else 
					{
						foreach ($sub_section->{"items"} as $item)
						{
							if($item->{"text"} != "" && $item->{"display"} == true)
							{
								$dispaly_section= true;
								break;
							}
						}
					}
					if($dispaly_section)
					{
						$this->Ln(1);
						if($sub_section->{"with_title"} == true && $sub_section->{"title"} != "" )
						{
							$title=$sub_section->{"title"};
							$titles = explode(":", $title);
							$title1=$titles[0];
							$title2=$titles[1];
							$title1 =strToUpper(utf8_decode($this->wd_remove_accents($title1)));
							if($title2 != "")
							{
								$title=$title1." : ".utf8_decode(trim($title2));
							}
							else
								$title=$title1;
							$this->SetFont($this->subsectiontitle_font,'',8);
							$this->setMyColText('GREEN');
							$this->MultiCell(0,3,$title,0,"L",false);	
						}

						//sub section description
						$description=iconv('UTF-8', 'windows-1252', stripslashes(html_entity_decode($sub_section->{"description"}, ENT_QUOTES | ENT_HTML5)));
						if($sub_section->{"with_description"} == true && $description != "" )
						{
							$this->SetFont($this->page_font,'',8);
							$this->setMyColText();
							$this->MultiCell(0,4,$description,"",3,'');
						}
						
						//sub section items
						foreach ($sub_section->{"items"} as $item)
						{
							//puces
							if($item->text != "")
							{
								$display_item=$item->{"display"};
								if($display_item == true)
								{
									if($section->name != "A savoir")
									{
										// remplacement de arial
										// $this->SetFont("helvetica",'B',8);
										// $this->SetTextColor(95,0,10,35);

										$this->SetFont($this->bullet_font,'',8);
										$this->setMyColText('GREEN');
										$this->Cell(2,4,">", 0, 0, 'L', false );
									}

									//text
									$this->SetFont($this->page_font,'',8);
									$this->setMyColText();
									$text=html_entity_decode($item->{"text"}, ENT_QUOTES | ENT_HTML5);
									$text=iconv('UTF-8', 'windows-1252', stripslashes($text));
									$this->MultiCell(0,4,ucfirst($text),0,'L');
								}
							}
						}
						$i++;
					}
				}
				$this->Ln(2);
				$this->setMyColText();
			}
			elseif($section->name == "Joindre un Praticien")
			{
				//section title
				if($offer->doctors != null)
				{
					$this->SetFont($this->sectiontitle_font,'',14);
					$this->setMyColText();
					$img = NX_DOC_ROOT."nx/skins/lightsky/images/livret_fpdf/tele_livret_cmyk.jpg";
					$f=fopen($img ,'rb');
						
					if($f)
					{
						$this->Cell(4);
						$this->MultiCell(0,5,utf8_decode($section->name),"",3);
						$x = $this->GetX();
						$y = $this->GetY()-5;
						$this->Image($img,$x,$y,4,4);
					}
					else
					{
							$this->MultiCell(0,5,utf8_decode($section->name),"",3);
							$this->Cell(0,4,strtoupper(utf8_decode($title)),"",3,'');
					}
					$this->Ln(1);

					//doctors
					foreach ($offer->doctors as $oid=>$doctor)
					{
						$civility=$doctor->civility;
						if($civility == '' || $civility == 'DR' )
						{
							if(strtolower($offer->title) == "kinésithérapie" || strtolower($offer->title) == "ostéopathie")
								$civility='';
							else
								$civility='Dr';
						}
						if($civility == 'MR' )
							$civility='M.';
						if($civility == 'MME' )
							$civility='Mme';
						if($civility == 'MLLE' )
							$civility='Mlle';

						$doctor_name=trim($civility." ".$this->formatName($doctor->first_name)." ".$doctor->formatDoctorName($doctor->last_name));
						$doctor_name = iconv('UTF-8', 'windows-1252', stripslashes($doctor_name));

						$doctor_cord="";
						if($doctor->phone != "" && $doctor->phone != NULL)
							$doctor_cord=$doctor->phone;
						if($doctor->email_mobile != "" && $doctor->email_mobile != NULL)
							$doctor_cord=($doctor_cord != '')?$doctor_cord." - ".$doctor->email_mobile : $doctor->email_mobile;

						$this->SetFont($this->page_font,'',8);
						$this->setMyColText();
						// $this->Cell(47,4,$doctor_name,0,0,'L');
						$this->Cell(46,4,$doctor_name,0,0,'L');
						if($doctor_cord != "")
							$this->Cell(3,4," : ",0,0,'L');
						$this->MultiCell(0,4,trim($doctor_cord),0,'L');
					}
				}
			}
		}
	
	}


	function setEstablishment($establishment) 
	{
		if($establishment == null || $establishment == "")
		{
			$establishment=html_entity_decode("Etablissement modèle", ENT_QUOTES | ENT_HTML5);
			$establishment=iconv('UTF-8', 'windows-1252', stripslashes($establishment));
		}
		else
		{
			//interpreter les entités html
			$establishment=html_entity_decode($establishment, ENT_QUOTES | ENT_HTML5);
			$establishment=iconv('UTF-8', 'windows-1252', stripslashes($establishment));
			
		}

		$this->header_title=$establishment;
		$this->establishment=$establishment;
	}

	function setOffer($offer) 
	{
		$this->offer=$offer;
		$this->title=$offer->title;
		$this->subtitle=$offer->subtitle;
	}

	function AddPage($orientation = '', $format = '', $rotationOrKeepmargins = false, $tocpage = false) 
	{
		//on génère la page en appelant la méthode 
        //parente addPage()
        parent::AddPage();
 
        // si un lien existe pour ce numéro de page, 
        //on l'associe à la page
        $page = $this->pageNo();
        if (isset($this->myLinks[$page])) {
            $this->setLink($this->myLinks[$page],-1,-1);
        }

		if($this->page_type == "index" || $this->page_type == "emergency")
		{
			$pdf_larg = $this->w;
			$pdf_long = $this->h;
			//$this->SetFillColor(20,7,0,4);
			$this->SetFillColor(14,5,0,2);
			$this->Rect(0,0, $pdf_larg, $pdf_long, 'F');
		}
		$this->drawSwallows(true,true,true,true);
    }

	// En-tête
	function Header()
	{
		if($this->PageNo() != 1)
		{
			$this->setHeader($this->header_title,$this->header_font,$this->header_size);
		}
	}
	   
	function setHeader($header_title,$header_font,$header_size)
	{
		if($this->page_type == "summary")
		{
			$this->SetDrawColor(0,0,0,100);
			$this->setMyColText();
			$this->SetFont($header_font,'',$header_size);
			$this->SetY($this->marginTop);

			$this->Cell(0,4,$header_title,0,0,'C');
			$this->Ln(3);
			$this->SetLineWidth(1);
			$this->Cell(0,3,"","B",0,''); 
			$this->Ln(7);
		}
		elseif($this->page_type == "offer" || $this->page_type == "pole" || $this->page_type == "pôle" || $this->page_type == "note" || $this->page_type == "last_cover" )
		{
			$this->SetDrawColor(0,0,0,100);
			$this->setMyColText();
			$this->SetFont($header_font,'',$header_size);
			$this->SetY($this->marginTop);

			$this->Cell(0,4,$header_title,0,0,'C');
			$this->Ln(3);
			$this->SetLineWidth(1);
			$this->Cell(0,3,"","B",0,'');
			$this->Ln(5);
		}
		elseif($this->page_type == 'offer') 
		{
			$this->SetDrawColor(14,5,0,100);
			$this->SetTextColor(14,5,0,100);
			$this->SetFont($this->header_font,'',$this->header_size);
			$this->setY($this->marginTop);

			$this->Cell(0,4,$this->header_title,0,0,'C');
			$this->Ln(4);
			$this->SetLineWidth(1);
			$this->Cell(0,3,"","B",0,'');
			$this->Ln(5);
		}
	}

	function writeHeader_emergency() {
		$this->SetDrawColor(14,5,0,100);
		$this->SetTextColor(14,5,0,100);
		$this->SetFont($this->header_font,'',$this->header_size);
		$this->setY($this->marginTop);

		$this->Cell(0,4,$this->header_title,0,0,'C');
		$this->Ln(4);
		$this->SetLineWidth(1);
		$this->Cell(0,3,"","B",0,'');
		$this->Ln(5);
	}

	// Pied de page
	function Footer()
	{
		if($this->display_footer == true)
		{
			if($this->PageNo() == $this->num_pages["summary"])
			{
				$coord=$this->summaryInfos["summary_coord"];
				$this->setMyColText();
				$this->SetFont($footer_font,'',10);
				$this->SetFillColor(14,5,0,2);
				//surimpression
				$this->SetTextColor(14,5,0,100);
				$y=220-$this->marginBottom-$this->marginImp;
				$footerH=23;
				$this->SetY($y-$footerH);
				$this->MultiCell(0,8,iconv('UTF-8', 'windows-1252', stripslashes($coord->{"adr1"})),"",'C',true);
				$this->MultiCell(0,3,iconv('UTF-8', 'windows-1252', stripslashes($coord->{"adr2"})),"",'C',true);
				$this->SetFont("HelveticaNeueLTStd-Th",'',10);
				if($coord->{"tel2"} != "")
				{
					$this->MultiCell(0,6,iconv('UTF-8', 'windows-1252', stripslashes("Secrétariat : ".$coord->{"tel2"})),"",'C',true);
				}
				else if($coord->{"tel"} != "")
					$this->MultiCell(0,6,"Standard : ".$coord->{"tel"},"",'C',true);

				if($coord->{"email"} != "")
					$this->MultiCell(0,6,"Email : ".$coord->{"email"},"",'C',true);

				$this->SetDrawColor(0,0,0,100);
				$this->SetLineWidth(1);
				$this->Cell(0,0,"","B",0,'');
			}
			elseif($this->PageNo() == $this->num_pages["emergency"])
			{
				//surimpression
				$this->SetDrawColor(14,5,0,100);
				$this->SetTextColor(14,5,0,100);
				$y=220-$this->marginBottom-$this->marginImp;
				$this->SetY($y);
				$this->SetFont($footer_font,'',8);
				$this->SetLineWidth(1.5);
				$this->Cell(0,0,"","B",0,'');
			}
			elseif($this->PageNo() == $this->num_pages["index"])
			{
				//surimpression
				$this->SetTextColor(14,5,0,100);
				$this->setMyColText();
				$this->SetFont($footer_font,'',8);
				$y=220-$this->marginBottom-$this->marginImp;
				$this->SetY($y);
				if($this->PageNo() != 1)
				{
					if($this->PageNo() <=9)
						$this->Cell(0,4,"0".$this->PageNo(),0,0,'C'); 
					else
						$this->Cell(0,4,$this->PageNo(),0,0,'C'); 
				}
			}
			else
			{
				$this->setMyColText();
				$this->SetFont($footer_font,'',8);
				$y=220-$this->marginBottom-$this->marginImp;
				$this->SetY($y);
				if($this->PageNo() != 1)
				{
					if($this->PageNo() <=9)
						$this->Cell(0,4,"0".$this->PageNo(),0,0,'C'); 
					else
						$this->Cell(0,4,$this->PageNo(),0,0,'C'); 
				}
			}
		}
		
	}

	function writeVerticalTitle($title) {
		if(strlen($title) > 20)
		{
			$titleFontSize = 43;
			$x=10+$this->marginLeft; // 20
		}
		else
		{
			$titleFontSize = 50;
			$x=15+$this->marginLeft;  // 25
		}

		$this->SetFont($this->title_font,'',$titleFontSize);

		$wTitle=$this->GetStringWidth($title);
		$this->RotatedText($x,$wTitle+20,$title,90);
	}

	// ============== PDF : TEXT AND DRAWING ===================

	function utf8decodeAndSlashes($s) {
		return iconv('UTF-8', 'windows-1252', stripslashes($s));
	}

	function  formatName($name)
	{
		$names = explode("-", $name);
		if($names[1] == "")
			return $name;
		$part1=ucfirst(strtolower($names[0]));
		$part2=ucfirst(strtolower($names[1]));
		$name=$part1."-".$part2;
		return $name;
	}

	function  getImageWidth($wImg,$hImg,$logo_y)
	{
		return ($wImg*$logo_y)/$hImg;
	}

	function replaceAccentsUTF8($s) {
		$patterns = array(
			utf8_decode('/[á|â|à|å|ä]/i') => 'a',
			utf8_decode('/[ð|é|ê|è|ë]/i') => 'e',
			utf8_decode('/[í|î|ì|ï]/i')=>'i',
			utf8_decode('/[ó|ô|ò|ø|õ|ö]/i')=>'o',
			utf8_decode('/[ú|û|ù|ü]/i') => 'u',
			utf8_decode('/æ/i')=>'ae',
			utf8_decode('/ç/i') => 'c',
			utf8_decode('/ß/i') => 's'
		);
	

		return preg_replace(array_keys($patterns), array_values($patterns), $s);
	}

	function wd_remove_accents($str, $charset='utf-8')
	{
		$str = htmlentities($str, ENT_NOQUOTES, $charset);
		$str = preg_replace('#&([A-za-z])(?:acute|cedil|caron|circ|grave|orn|ring|slash|th|tilde|uml);#', '\1', $str); //suppriemer les caracteres accentues
		$str = str_replace('&rsquo;', '\'', $str);
		$str = preg_replace('#&([A-za-z]{2})(?:lig);#', '\1', $str); // pour les ligatures e.g. '&oelig;'
		$str = preg_replace('#&[^;]+;#', '', $str); // supprime les autres caractères
		return $str;
	}

	function setElementFont($element,$font)
	{
		if($font != "arial" && $font != "courier" && $font != "helvetica")
		{
			$this->AddFont($font,'');
		}
		$this->$element=$font;

	}

	function setMyColText($col=null) {
		if($col == 'GREEN')
			//$this->SetTextColor(100,0,11,30);
			$this->SetTextColor(100,0,50,10);
		elseif($col == 'BLUE')
			$this->SetTextColor(100,33,0,29);
		else
			$this->SetTextColor(0,0,0,100);
	}

	function drawLine($x,$y,$lenth)
	{
		//surimpression
		$this->SetDrawColor(14,5,0,100);
		$this->SetLineWidth(0);
		while($x <= $lenth)
		{
			$l=1;
			$this->Line($x,$y,$x+$l,$y);
			$x=$x+$l+1;
		}
	}

	function drawSwallows()
	{
		if($this->marginImp != 0)
		{
			$this->SetDrawColor(100, 100, 100, 100);
			$this->SetLineWidth(0);

			// coin top left
			$y1=0;
			$y2=$this->marginImp-2;
			$x=$this->marginImp;
			$this->Line($x,$y1,$x,$y2);

			$x1=0;
			$x2=$this->marginImp-2;
			$y=$this->marginImp;
			$this->Line($x1,$y,$x2,$y);

			// coin top right
			$y1=0;
			$y2=$this->marginImp-2;
			$x=115-$this->marginImp;
			$this->Line($x,$y1,$x,$y2);

			$y1=$this->marginImp;
			$x1=115-$this->marginImp+2;
			$x2=115;
			$this->Line($x1,$y,$x2,$y);

			// coin bottom left
			$x1=0;
			$y=220-$this->marginImp;
			$x2=$this->marginImp-2;
			$this->Line($x1,$y,$x2,$y);

			$x=$this->marginImp;
			$y1=220;
			$y2=220-$this->marginImp+2;
			$this->Line($x,$y1,$x,$y2);


			// coin bottom right
			$x1=115-$this->marginImp+2;
			$y=220-$this->marginImp;
			$x2=115;
			$this->Line($x1,$y,$x2,$y);

			$x=115-$this->marginImp;
			$y1=220;
			$y2=220-$this->marginImp+2;
			$this->Line($x,$y1,$x,$y2);
		}	
	}

	function Rotate($angle,$x=-1,$y=-1)
	{
		if($x==-1)
			$x=$this->x;
		if($y==-1)
			$y=$this->y;
		if($this->angle!=0)
			$this->_out('Q');
		$this->angle=$angle;
		if($angle!=0)
		{
			$angle*=M_PI/180;
			$c=cos($angle);
			$s=sin($angle);
			$cx=$x*$this->k;
			$cy=($this->h-$y)*$this->k;
			$this->_out(sprintf('q %.5F %.5F %.5F %.5F %.2F %.2F cm 1 0 0 1 %.2F %.2F cm',$c,$s,-$s,$c,$cx,$cy,-$cx,-$cy));
		}
	}

	function _endpage()
	{
		if($this->angle!=0)
		{
			$this->angle=0;
			$this->_out('Q');
		}
		parent::_endpage();
	}

	function RotatedText($x,$y,$txt,$angle)
	{
		//Text rotated around its origin
		$this->Rotate($angle,$x,$y);
		$this->Text($x,$y,$txt);
		$this->Rotate(0);
	}

	function RotatedImage($file,$x,$y,$w,$h,$angle)
	{
		//Image rotated around its upper-left corner
		$this->Rotate($angle,$x,$y);
		$this->Image($file,$x,$y,$w,$h);
		$this->Rotate(0);
	}

	function Circle($x, $y, $r, $style='D')
	{
		$this->Ellipse($x,$y,$r,$r,$style);
	}

	function Ellipse($x, $y, $rx, $ry, $style='D')
	{
		if($style=='F')
			$op='f';
		elseif($style=='FD' || $style=='DF')
			$op='B';
		else
			$op='S';
		$lx=4/3*(M_SQRT2-1)*$rx;
		$ly=4/3*(M_SQRT2-1)*$ry;
		$k=$this->k;
		$h=$this->h;
		$this->_out(sprintf('%.2F %.2F m %.2F %.2F %.2F %.2F %.2F %.2F c',
			($x+$rx)*$k,($h-$y)*$k,
			($x+$rx)*$k,($h-($y-$ly))*$k,
			($x+$lx)*$k,($h-($y-$ry))*$k,
			$x*$k,($h-($y-$ry))*$k));
		$this->_out(sprintf('%.2F %.2F %.2F %.2F %.2F %.2F c',
			($x-$lx)*$k,($h-($y-$ry))*$k,
			($x-$rx)*$k,($h-($y-$ly))*$k,
			($x-$rx)*$k,($h-$y)*$k));
		$this->_out(sprintf('%.2F %.2F %.2F %.2F %.2F %.2F c',
			($x-$rx)*$k,($h-($y+$ly))*$k,
			($x-$lx)*$k,($h-($y+$ry))*$k,
			$x*$k,($h-($y+$ry))*$k));
		$this->_out(sprintf('%.2F %.2F %.2F %.2F %.2F %.2F c %s',
			($x+$lx)*$k,($h-($y+$ry))*$k,
			($x+$rx)*$k,($h-($y+$ly))*$k,
			($x+$rx)*$k,($h-$y)*$k,
			$op));
	}

}

class OFFER 
{
	var $oid;
	var $title;
	var $subtitle;
	var $type;
	var $index;
	var $display;
	var $in_summary;
	var $summary_index;
	var $iconFile;
	var $sections;
	var $doctors;

	function OFFER($oid,$title,$subtitle,$index,$display,$in_summary=false,$summary_index=0,$summary_alias=null,$iconFile=null)
	{
		
		//DebugBreak();
		$this->oid=$oid;
		
		$search = array("\t", "\n", "\r");
		$title = trim(str_replace($search, '', $title));
		$subtitle = trim(str_replace($search, '', $subtitle));


		if($subtitle != "" && $title != "")
			$title = str_replace($subtitle, "", $title);

		$this->index=$index;
		$this->title=$title;
		$this->subtitle=$subtitle;
		$this->display=$display;
		$this->in_summary=$in_summary;
		$this->summary_index=$summary_index;
		if($summary_alias == null || $summary_alias == "")
			$this->summary_alias=$title;
		else
			$this->summary_alias=$summary_alias;
		$this->iconFile=$iconFile;
	}

	function getTitle()
	{
		return $this->title;
	}

	function setSections($sections)
	{
		$this->sections=$sections;
	}

	function setDoctors($doctors)
	{
		$this->doctors=$doctors;
	}

	function addSection($section)
	{
		$this->sections[]=$section;
	}

	function getSections()
	{
		return $this->sections;
	}

	static function compare($a, $b)
	{
	   if ($a->index == $b->index) {
		 return 0;
	   }
	   return ($a->index < $b->index) ? -1 : 1;
	}

	static function compare_summary_index($a, $b)
	{
	   if ($a->summary_index == $b->summary_index) {
		 return 0;
	   }
	   return ($a->summary_index < $b->summary_index) ? -1 : 1;
	}
	
}

class SECTION
{
	var $oid;
	var $name;
	var $sub_sections;

	function SECTION($name,$sub_sections)
	{
		$this->name=$name;
		$this->sub_sections=$sub_sections;
	}
	
}

class DOCTOR
{
	var $oid;
	var $civility;
	var $first_name;
	var $last_name;
	var $phone;
	var $phone2;

	function DOCTOR($oid,$civility,$first_name,$last_name,$phone)
	{
		$this->oid=$oid;
		$this->civility=$civility;
		$this->first_name=$first_name;
		$this->last_name=strtoupper($last_name);
		$this->phone=$this->formatPhone($phone);
		//$this->phone2=$this->formatPhone($phone2);
		$this->email_mobile='';
	}

	function formatPhone($phone)
	{
		if($phone == 0)
			return ;
		return str_replace('.',' ',$phone);
	}
	
	function formatDoctorName($last_name)
	{
		$parts = explode(" ",$last_name);
		if(count($parts) > 1 && strtolower($parts[0]) == 'de')
		{
			$last_name="";
			for($i=0; $i<count($parts);$i++)
			{
				if($i==0)
					$last_name=" ".strtolower($parts[$i]);
				else
					$last_name.=" ".strtoupper($parts[$i]);
			}
			return trim($last_name);
		}else
			return $last_name;
	}
	
	static function compare($a, $b)
	{
	   if ($a->last_name == $b->last_name) {
		 return 0;
	   }
	   return ($a->last_name < $b->last_name) ? -1 : 1;
	}
}