<?php

require(NX_LIB.'fpdf/fpdf.php');
require(NX_LIB.'fpdi/fpdi.php');

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
		$this->setElementFont('sectiontitle_font','HelveticaNeueLTStd-Md');
		$this->setElementFont('subsectiontitle_font','HelveticaNeueLTStd-Md');


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

			$logo_y=$logo_y+$this->marginTop-$this->marginBottom;
			if($logo_x <0)
			{
				$logo_x	=$logo_x-$logo_w;
				$logo_x=$wPdf+$logo_x;
				$logo_x=$logo_x+$this->marginImp;
			}
			else
				$logo_x	=$logo_x-$lMargin;

			//$this->Cell($logo_x);
			$this->Image($logo,$logo_x,$logo_y,"",$logo_h); 
		}
	}

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
				$this->Image($img,$bg_x+2,$bg_y+$this->marginTop,115-2-2,120);
			}

			//logo
			$this->displayLogo($cover_info["logo"],$logo_x = $cover_info["logo_x"],$logo_y = $cover_info["logo_y"],$cover_info["logo_height"]);

			//logo groupe
			$this->displayLogo($cover_info["logo2"],$logo_x = $cover_info["logo2_x"],$logo_y = $cover_info["logo2_y"],$cover_info["logo2_height"]);

			$title =$cover_info["title"];
			$subtitle =$cover_info["subtitle"];
			$titre_edition =$cover_info["titre_edition"];

			
			//titre
			$this->SetFont($this->cover_font,'',21);
			$this->setTextColorByName('blue_title');;
			$x=$cover_info["title_x"];
			$y=$cover_info["title_y"];
			$this->SetY($y+$this->marginTop);
			$this->MultiCell(0,12,$title,0,'C');
			$this->Ln(0);

			//sous titre
			$this->SetFont($this->cover_font,'',15);
			$x=$cover_info["subtitle_x"];
			$y=$cover_info["subtitle_y"];
			$this->SetY($y+$this->marginTop);
			$this->Cell(0,12,$subtitle,0,0,'C');
			

			//edition
			$x=$cover_info["edition_x"];
			$y=$cover_info["edition_y"];
			$this->SetXY($x+$this->marginLeft,$y+$this->marginTop);
			$this->SetFont($this->cover_font,'',12);
			$this->setTextColorByName('black');;
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
			$this->SetMargins(10,0,10);

			$this->Ln(4);
			$this->SetFont($this->title_font,'',14);
			$this->SetTextColor(100,29,0,10);
			$title="VOUS VOULEZ CONNAÎTRE LES FORMATIONS ORGANISÉES PAR LES PRATICIENS ET LES NOUVEAUTÉS AU SEIN DE L’ÉTABLISSEMENT (SPÉCIALITÉ, SERVICE, MATÉRIEL…) ?,";
			$title=html_entity_decode($title, ENT_QUOTES | ENT_HTML5);
			$title=iconv('UTF-8', 'windows-1252', stripslashes($title));
			$this->MultiCell(0,6,$title,"",C,'');
			$this->Ln(1);

			$this->Ln(2);
			$this->setTextColorByName('black');;
			$text="Et bientôt contacter directement l’un de vos confrères ou disposer d’informations actualisées et sécurisées sur la prise en charge de vos patients.";
			$text=html_entity_decode($text, ENT_QUOTES | ENT_HTML5);
			$text=iconv('UTF-8', 'windows-1252', stripslashes($text));
			$this->SetFont($this->title_font,'',13);
			$this->MultiCell(0,5,$text,"",C,'');
			$this->Ln(4);

			$y=$this->getY();
			$img = NX_DOC_ROOT."/nx/skins/lightsky/images/livret_fpdf/contact.png";
			$f=fopen($img ,'rb');
			if($f)
			{
				$this->Image($img,$this->getX()+3,$this->getY(),95,75);
			}

			$this->setY($y+13);
			$this->setX($this->getX()+20);
			$this->SetFont($this->title_font,'',13);
			$this->SetTextColor(100,29,0,10);
			$title="Rendez-vous sur";
			$title=html_entity_decode($title, ENT_QUOTES | ENT_HTML5);
			$title=iconv('UTF-8', 'windows-1252', stripslashes($title));
			$this->MultiCell(60,3,$title,"",L,'');
			$this->Ln(1);
			$this->setX($this->getX()+20);
			$this->SetFont($this->page_font,'',12);
			$this->setTextColorByName('black');;
			$url="www.ramsaygds.com";
			$url=html_entity_decode($url, ENT_QUOTES | ENT_HTML5);
			$url=iconv('UTF-8', 'windows-1252', stripslashes($url));
			$this->MultiCell(60,6,$url,"",L,'');
			$this->Ln(1);
			$this->setX($this->getX()+20);
			$this->SetFont($this->title_font,'',13);
			$this->SetTextColor(100,29,0,10);
			$title="Laissez_nous";
			$title=html_entity_decode($title, ENT_QUOTES | ENT_HTML5);
			$title=iconv('UTF-8', 'windows-1252', stripslashes($title));
			$this->MultiCell(60,3,$title,"",L,'');
			$this->Ln(1);
			$this->setX($this->getX()+20);
			$title="Votre adresse email.";
			$title=html_entity_decode($title, ENT_QUOTES | ENT_HTML5);
			$title=iconv('UTF-8', 'windows-1252', stripslashes($title));
			$this->MultiCell(60,3,$title,"",L,'');
		}

	}

	function generateSummaryPage()
	{
		$this->AddPage();
		$this->ln(1);
		$this->setTextColorByName('black');;
		$this->SetFont($this->page_font,'',13);

		$establishment=$this->establishment;
		if(preg_match("#^[aeiouyhAEIOUYH]#", $establishment))
		   $establishment="L' ".$establishment;
		else
		   $establishment="La ".$establishment;

		$summaryText=$establishment."".utf8_decode(" appartient au pôle ");
		$poleName = $this->summaryInfos["pole"];
		$summaryText=$summaryText.' '.utf8_decode("{$poleName} du premier groupe français de soins et services à la santé, Ramsay Santé.");
		$this->MultiCell(0,5,$summaryText,"",L,'');
		$this->ln(1);
		$this->SetFont('HelveticaNeueLTStd-Md','',13);
		$this->MultiCell(0,5,utf8_decode("Son activité repose sur plusieurs spécialités fortes :"),"",L,'');
		$this->ln(4);

		if($this->summaryInfos != null)
		{
			$bed_number=$this->summaryInfos["bed_number"];
			$bed_number_desc=$this->summaryInfos["bed_number_desc"];
			$nb_doctors=$this->summaryInfos["nb_doctors"];
			$offers=$this->summaryInfos["offers"];
		}
		else
		{
			return;
		}

		$currentPosition=$this->displaySummaryOffers($offers);

		$this->displaySummaryInfo($img_bed_numbers,$bed_number,$bed_number_desc,$img_doctors,$nb_doctors,$currentPosition);
		
	}

	function displaySummaryOffers($offers)
	{
		//DebugBreak();
		$this->SetFont('HelveticaNeueLTStd-Md','',8);
		$this->setTextColorByName('black');;
		
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
		
		for ($i=0;$nbOffers>$i;$i++)
		{
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
		}

		return $currentY+5;
	}


	function displaySummaryInfo($img_bed_numbers,$bed_number,$bed_number_desc,$img_doctors,$nb_doctors,$currentPosition)
	{
		$this->Ln(1);
		$this->SetFont('HelveticaNeueLTStd-Md','',8);
		$currentY=$currentPosition;
		$this->SetY($currentY+20);

		$img_bed_numbers = NX_DOC_ROOT."/nx/skins/lightsky/images/livret/bed_numbers.png";
		$img_doctors = NX_DOC_ROOT."/nx/skins/lightsky/images/livret/doctors_nb.png";
		$f_bed_numbers=fopen($img_bed_numbers,'rb');
		$f_doctors=fopen($img_doctors  ,'rb');
		if($f_bed_numbers && $f_doctors)
		{
			//$this->SetX(20);
			if($bed_number != "")
				$this->Cell(50,12,$this->Image($img_bed_numbers,30,$this->GetY(),12,12),0,"","C");
			if($nb_doctors != "")
				$this->Cell(50,12,$this->Image($img_doctors,80,$this->GetY(),12,12),0,"","C");
			$this->ln();
		}

		$this->SetTextColor(85,22,0,11);
		$this->SetFontSize(22);
		//$this->SetX(20);
		if($bed_number != "")
			$this->Cell(50,8,$bed_number,0,"","C");
		if($nb_doctors != "")
			$this->Cell(50,8,$nb_doctors,0,"","C");
		$this->ln();

		
		$this->SetFontSize(8);
		if($bed_number != "")
		{
			$text=($bed_number_desc != "") ? "lits et places dont" : "lits et places";
			$this->Cell(50,3,$text,'',"","C");
		}
		if($nb_doctors != "")
			$this->Cell(50,3,"praticiens",'',"","C");
		$this->ln();

		$this->setTextColorByName('black');;
		if($bed_number != "")
			$this->MultiCell(50,3,$bed_number_desc,0,"C");
		$this->ln();
	}

	function generateIndex()
	{
		$offers_list=$this->offersList;
		$this->AddPage();
		

		//header
		$this->setTextColorByName('black');;
		$this->SetFont($this->header_font,'',$this->header_size);
		$this->setY($this->marginTop);
		$this->Cell(0,4,$this->header_title,0,0,'C');
		$this->Ln(4);
		$this->SetLineWidth(1);
		$this->Cell(0,3,"","B",0,'');
		$this->Ln(5);

		$numPage=$this->PageNo()+1;
		$this->SetTextColor(43,14,0,8);
		$this->SetFont($this->title_font,'',43);
		$this->RotatedText(20,200,'Index des offres de soins ',90);
		$this->ln(6);
		
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
				$this->setTextColorByName('black');;
				$this->cell(75,2,$titrePage,0,0,'L',false,$this->myLinks[$numPage]);
				//on écrit le numéro de la page
				if($numPage <=9)
					$this->cell(4,2,"0".$numPage,0,2,'R',false);
				else
					$this->cell(4,2,$numPage,0,2,'R',false);
				$this->ln();
				$numPage++;
			}
		}

	}

	function generateEmergencyPDF()
	{
		//********data*******;
		$urgences=$this->emergencyInfos["urgencies"];
		$praticiens=$this->emergencyInfos["praticiens"];
		$data = array();
		if($this->emergencyInfos["passages"] != "" && $this->emergencyInfos["passages_text"] != "")
			$data[]=array("number" => $this->emergencyInfos["passages"], "text" => $this->emergencyInfos["passages_text"]);
		if($this->emergencyInfos["nb_beds"] != "" && $this->emergencyInfos["nb_bed_text"] != "")
			$data[]=array("number" => $this->emergencyInfos["nb_beds"], "text" => iconv('UTF-8', 'windows-1252', stripslashes($this->emergencyInfos["nb_bed_text"])));
		if($this->emergencyInfos["boxes"] != "" && $this->emergencyInfos["box_text"] != "")
			$data[]=array("number" => $this->emergencyInfos["boxes"], "text" => iconv('UTF-8', 'windows-1252', stripslashes($this->emergencyInfos["box_text"])));

		
		
		$this->AddPage();
		$this->setTextColorByName('black');;
		$this->SetFont($this->header_font,'',$this->header_size);
		$this->setY($this->marginTop);
		$this->Cell(0,4,$this->header_title,0,0,'C');
		$this->Ln(4);
		$this->SetLineWidth(1);
		$this->Cell(0,3,"","B",0,'');
		$this->Ln(5);

		$this->SetTextColor(43,14,0,8);
		$this->SetFont($this->title_font,'',50);
		$title=utf8_decode($this->emergencyInfos["page_title"]);
		$wTitle=$this->GetStringWidth($title);
		$this->RotatedText(15+$this->marginLeft,$wTitle+20,$title,90);
		$margin=27;

		if(count($urgences) != 0)
		{
			foreach($urgences as $urgence)
			{
				$this->Ln(2);
				$this->setX($margin);
				$img_fleche = NX_DOC_ROOT."nx/skins/lightsky/images/livret_fpdf/fleche.png";
				$f=fopen($img_fleche ,'rb');
				if($f)
				{
					$this->Image($img_fleche ,$this->Getx()+1,$this->GetY()+1,3,3);
				}
				$this->setX($margin+4);
				$this->SetFont($this->page_font,'',10);
				$this->SetTextColor(100,31,0,15);
				$this->MultiCell(0,4,$urgence["title"],'0','L','');

				if($urgence["display_description"] && $urgence["description"] != "")
				{
					$this->setTextColorByName('black');;
					$this->setX($margin+4);
					$this->SetFont($this->page_font,'',10);
					$this->MultiCell(0,6,$urgence["description"],'0','L','');
				}
				
				
				foreach ($urgence["contacts"] as $contact)
				{
					if($contact->{'label'})
					{
						$this->setX($margin+4);
						$this->SetFont($this->page_font,'U',8);
						$this->setTextColorByName('black');;
						$this->Cell(30,4,utf8_decode($contact->{'label'}." : "),0,0,'L');
						$this->SetFont($this->title_font,'',8);
						$this->Cell(40,4,utf8_decode($contact->{'text'}),'0','1','L');
					}
				}

			}
			$this->Ln(2);
			$this->drawLine($margin+$this->marginImp,$this->getY(),100);
		}


		if(count($data) != 0)
		{
			$this->Ln(2);
			$this->SetTextColor(85,22,0,11);
			foreach ($data as $element)
			{
				$this->setX($margin+$this->marginImp);
				$this->SetFont($this->title_font,'',30);
				$w=$this->GetStringWidth($element["number"]);

				$this->Cell($w+3,10,$element["number"],0,0,'L');
				$this->SetFont($this->title_font,'',8);
				$this->setXY($margin+$this->marginImp+$w+2,$this->getY()+4);
				$this->Multicell(0,4,$element["text"],0,1,'L');
				$this->Ln(2);
			}
			$this->drawLine($margin+$this->marginImp,$this->getY(),100);
		}
		
		
		if(count($praticiens) != 0)
		{
			$this->SetFont($this->title_font,'',12);
			$this->SetTextColor(0,0,0,0);
			$text="Joindre un Praticien";
			$img = NX_DOC_ROOT."nx/skins/lightsky/images/livret_fpdf/phone-volume.jpg";
			$f=fopen($img ,'rb');
			if($f)
			{
				$this->Image($img,$margin,$this->GetY(),6,6);
				$this->setXY($margin+8,$this->GetY());
				$this->MultiCell(0,5,utf8_decode($text),"",3);
			}
			else
			{
				$this->setX($margin);
				$this->MultiCell(0,5,utf8_decode($text),"",3);
			}

			$this->Ln(2);
			foreach ($praticiens as $doctor)
			{
				$this->setX($margin+5);
				$doctor_name="Dr ".$doctor->first_name." ".strToUpper($doctor->last_name);
				$doctor_name = iconv('UTF-8', 'windows-1252', stripslashes($doctor_name));
				$doctor_cord="";
				if($doctor->phone != "")
				{
					$doctor_cord=" ".$doctor->phone;
				}
				if($doctor->phone2 != "")
				{
					$doctor_cord=$doctor_cord." ".$doctor->phone2;
				}
				$this->SetFont($this->page_font,'',8);
				$this->SetTextColor(0,0,0,0);
				$this->Cell(30,5,$doctor_name,0,0,'L');
				//$this->MultiCell(42,4,$doctor_name,"",1);
				if($doctor_cord != "")
					$this->Cell(1,5,":",0,0,'L');
				$this->Cell(30,4,$doctor_cord,0,1,'L');

			}
			$this->Ln(2);
			$this->drawLine($margin+$this->marginImp,$this->getY(),100);
		}

	}

	function drawLine($x,$y,$lenth)
	{
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

	function setPoleTitle()
	{
		$this->Ln(2);
		$this->SetFont($this->title_font,'',20);
		$this->SetTextColor(100,31,0,15);
		$this->MultiCell(0,10,utf8_decode("Le Pôle"),'0','L','');
		$this->setTextColorByName('black');;
		$this->Ln(1);
		$cellWith=115-$this->margin100Left-$this->marginRight-($this->marginImp*2);
		$img=$this->poleInfos["map_file"];
		$img=str_replace("/afiles",NX_CLIENT_ROOT."files",$img);
		$f=fopen($img ,'rb');
		if($f)
		{
			$this->Image($img,30+$this->marginLeft+$this->marginImp,20,60);
		}
		$this->SetFont($this->subtitle_font,'',30);
		$this->MultiCell(60,9,utf8_decode($this->poleInfos["pole_name"]),"0","L",false);
		$this->SetLineWidth(1);
		$this->Cell(35,2,"","B",0,'');
		
	}

	function generatePolePDF()
	{
		$this->AddPage();
		//data
		$nb_hp=$this->poleInfos["nb_hp"];
		$img_hp = NX_DOC_ROOT."/nx/skins/lightsky/images/livret/hp_nb.png";
		$img_places = NX_DOC_ROOT."/nx/skins/lightsky/images/livret/bed_numbers_pole.png";

		//titre
		$this->setPoleTitle();

		$this->SetFont('HelveticaNeueLTStd-Md');
		$this->SetTextColor(0,0,0,0);
		$this->SetFillColor(100,31,0,15);
		$this->SetDrawColor(100,31,0,15);

		
		//nombre etablissement et lit
		$y_pos = 72;
		$x_pos = $this->GetX();
		$this->SetY($y_pos);
		if(fopen($img_hp,'rb') && fopen($img_places,'rb'))
		{
			$this->Cell(0,19,"",0,0,"L",TRUE);
			$this->setXY($x_pos,$y_pos);
			$this->Image($img_hp,$this->marginLeft+1,$y_pos+6,13,10,'PNG');
			
			$this->setXY(24,$y_pos);
			$x = $this->GetX();
			$this->SetFont('HelveticaNeueLTStd-Md','',20);
			$this->Cell(5,18,$nb_hp,0,0,"L",TRUE);
			$this->SetFont("HelveticaNeueLTStd-Th",'',9);
			$this->setXY($x+5,$y_pos+7);
			if($nb_hp > 1)
				$nb_hp_txt="établissements";
			else
				$nb_hp_txt="établissement";

			$this->Cell(32,5,utf8_decode($nb_hp_txt),0,0,"L",TRUE);
			$this->setXY($x,$y_pos+12);
			$this->Cell(36,4,utf8_decode(""),0,0,"L",TRUE);
			
			$this->setXY($this->GetX(),$y_pos);
			$x=$this->GetX();
			$this->SetFont('HelveticaNeueLTStd-Md','',20);
			$this->Cell(10,10,"",0,0,"L",true);
			$this->Image($img_places,$x,$y_pos+6,10,10,'PNG');
			$this->setXY($this->GetX(),$y_pos+8);
			$this->Cell(15,10,$this->poleInfos["nb_beds"],0,0,"L",TRUE);
			$this->setXY($this->GetX(),$y_pos+9);
			$this->SetFont("HelveticaNeueLTStd-Th",'',9);
			$this->Cell(15,10,"lits et places",0,0,"L",TRUE);
		}

		$y_pos=$y_pos+23;
		$this->SetXY($this->marginLeft,$y_pos);
		$this->setTextColorByName('black');;
		$this->SetFont('HelveticaNeueLTStd-Md','',9);
		$description= $this->poleInfos["description"];
		if($description != "")
			$this->MultiCell(0,4,$description,0,'J',false);

		$y_pos=$this->GetY();
		$this->SetY($y_pos);
		$sites= $this->poleInfos["sites"];

		$this->Ln(1);
		$i=1; //numerotation
		//sites
		foreach($sites as $site)
		{
			$name=iconv('UTF-8', 'windows-1252', stripslashes($site->{name}));
			if($site->{type} == "Etablissement" && $site->{selected} == TRUE && $name == $this->establishment)
			{
				$this->displayPoleSite($site,$i);
				$i++;
			}
			
		}

		foreach($sites as $site)
		{
			$name=iconv('UTF-8', 'windows-1252', stripslashes($site->{name}));
			if($site->{type} == "Etablissement" && $site->{selected} == TRUE && $name != $this->establishment)
			{
				$this->displayPoleSite($site,$i);
				$i++;
			}
			
		}
		$count=0; 
		foreach($sites as $site)
		{
			if($site->{type} == "Centre Imagerie" && $site->{selected} == TRUE)
			{
				$count++;
			}
		}
		if($count >0)
		{
			$this->setTextColorByName('black');;
			$this->SetFont('HelveticaNeueLTStd-Th','',20);
			if(count($sites) >=4)
			{
				$this->AddPage();
				$this->Ln(3);
			}
			else
			{
				$this->Ln(6);
			}

			$title=iconv('UTF-8', 'windows-1252', stripslashes("Sociétés d’Imagerie Médicale"));
			$this->MultiCell(0,4,$title,0,'L',false);
			$this->Ln(3);
			$i=1; //numerotation
			foreach($sites as $site)
			{
				if($site->{type} == "Centre Imagerie" && $site->{selected} == TRUE)
				{
					$this->displayPoleSite($site,$i);
					$i++;
				}
			}
		}

		$this->SetDrawColor(0,0,0,0);
	}

	function displayPoleSite($site,$number)
	{
		$this->Ln(3);
		$img = NX_DOC_ROOT."nx/skins/lightsky/images/livret_fpdf/chiffre.jpg";
		if(fopen($img,'rb'))
		{
			$this->SetFont($this->title_font,'',8);
			$this->SetTextColor(0,0,0,0);

			$x=$this->GetX();
			//$y=$this->GetY();
			$this->Cell(5,5,utf8_decode($number),0,0,'C','');
			$this->Image($img,$x,$this->GetY(),5,5);
			$this->SetX($x);
			$this->Cell(5,5,utf8_decode($number),0,0,'C','');
				
		}

		$this->SetFont($this->title_font,'',14);
		$this->SetTextColor(100,31,0,15);
		$name=iconv('UTF-8', 'windows-1252', stripslashes($site->{name}));
		$this->MultiCell(0,5,$name,'0','L','');
		$this->Ln(2);

		$this->setTextColorByName('black');;
		$this->SetFont("HelveticaNeueLTStd-Th",'',8);
		if($site->{address_line1} != null && $site->{address_line1} != "")
			$this->MultiCell(0,3,utf8_decode($site->{address_line1})." ".utf8_decode($site->{cp})." ".utf8_decode($site->{city}),'0','L','');

				
		if($site->{phone} != "" && $site->{phone} != null)
		{
			$this->Ln(1);
			$x=$this->GetX();
			$y=$this->GetY();
			$this->Image($img,$this->GetX()+1,$this->GetY()+1,1,1);
			$this->SetXY($x+2,$y);
			$this->Cell(0,3,utf8_decode("Tél. : ".$site->{phone}),0,1,'L','');
		}
		if($site->{email} != "" && $site->{email} != null)
		{
			$this->Ln(1);
			$x=$this->GetX();
			$y=$this->GetY();
			$this->Image($img,$this->GetX()+1,$this->GetY()+1,1,1);
			$this->SetXY($x+2,$y);
			$this->Cell(0,3,utf8_decode("Email. : ".$site->{email}),0,1,'L','');
		}
	}

	// set text colour in CMYK
	function SetTextColor_CMYK($c,$m,$y,$k) {
		$this->TextColor = sprintf('%.2F %.2F %.2F %.2F k', $c / 100, $m / 100, $y / 100, $k / 100);
		$this->ColorFlag = ($this->FillColor != $this->TextColor);
		nxError("FPDF TEXT CMYK COLOR = ($c,$m,$y,$k) ".$this->TextColor);
	}

	function generateNotePDF()
	{
		//$this->display_footer=false;
		$this->AddPage();

		$this->SetTextColor_CMYK(0,0,0,29);
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

	function generateOfferPDF()
	{
		$this->AliasNbPages();
		$this->AddPage();
	
		//content
		$this->setOfferTitle();
		$this->setSections($this->offer->getSections());
	}

	function setMyColText($col=null) {
		if($col == 'GREEN')
			//$this->SetTextColor(100,0,11,30);
			$this->SetTextColor(100,0,50,10);
		else
			$this->setTextColorByName('black');;
	}

	function setOfferTitle()
	{
		$this->SetFont($this->title_font,'',$this->title_size);
		//$this->SetTextColor(65,0,0,31);
		$this->setMyColText('GREEN');
		if(!$this->subtitle)
		{
			$this->MultiCell(0,10,utf8_decode($this->title),'0','L',false);
			$this->setTextColorByName('black');;
			$this->SetLineWidth(0);
			$this->Cell(0,-1,"","B",0,'');
			$this->Ln(1);
		}
		else
		{
			$this->MultiCell(0,10,utf8_decode($this->title),'0','L','');
			$this->setTextColorByName('black');;
			$this->SetFont($this->subtitle_font,'',$this->subtitle_size);
			$this->Ln(1);
			$this->MultiCell(0,6,utf8_decode($this->subtitle),"0","L",false);
			$this->SetLineWidth(0);
			$this->Cell(0,2,"","B",0,'');
			$this->Ln(7);
		}
		
		
	}

	function replaceAccentsUTF8($s) {
		$patterns[0] = '/[á|â|à|å|ä]/';
		$patterns[1] = '/[ð|é|ê|è|ë]/';
		$patterns[2] = '/[í|î|ì|ï]/';
		$patterns[3] = '/[ó|ô|ò|ø|õ|ö]/';
		$patterns[4] = '/[ú|û|ù|ü]/';
		$patterns[5] = '/æ/';
		$patterns[6] = '/ç/';
		$patterns[7] = '/ß/';
	
		$replacements[0] = 'a';
		$replacements[1] = 'e';
		$replacements[2] = 'i';
		$replacements[3] = 'o';
		$replacements[4] = 'u';
		$replacements[5] = 'ae';
		$replacements[6] = 'c';
		$replacements[7] = 'ss';

		return preg_replace($patterns, $replacements, $s);
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
				$this->setTextColorByName('black');;
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

						$title=$sub_section->{"title"};
						if($title != "" )
						{
							$titles = explode(":", $title);
							$title1=$titles[0];
							$title2=$titles[1];
							$title1 = strToUpper($title1);
							$title1 =$this->replaceAccentsUTF8(utf8_decode($title1));
							if($title2 != "")
							{
								$title=$title1." : ".utf8_decode(trim($title2));
							}
							else
								$title=$title1;

							$this->SetFont($this->subsectiontitle_font,'',8);
							// $this->SetTextColor(95,0,10,35);
							$this->setMyColText('GREEN');

							$this->MultiCell(0,3,$title,0,"L",false);	
						}

						//sub section description
						$description=utf8_decode($sub_section->{"description"});
						if($description != "" )
						{
							$this->SetFont($this->page_font,'',8);
							$this->setTextColorByName('black');;
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
										$this->SetFont("arial",'B',8);
										// $this->SetTextColor(95,0,10,35);
										$this->setMyColText('GREEN');

										$this->Cell(2,4,">", 0, 0, 'L', false );
									}

									//text
									$this->SetFont($this->page_font,'',8);
									$this->setTextColorByName('black');;
									$search = array("\t", "\n", "\r");
									$item->{"text"} = trim(str_replace($search, '', $item->{"text"}));
									$text = stripslashes($item->{"text"});
									$text = iconv('UTF-8', 'windows-1252', $text);
									$this->MultiCell(0,4,$text,"",3,'');
								}
							}
						}
						
						$i++;
						
					}
					
				}

				$this->Ln(2);
				$this->setTextColorByName('black');;
			}
			elseif($section->name == "Joindre un Praticien")
			{
				//section title
				if($offer->doctors != null)
				{
					$this->SetFont($this->sectiontitle_font,'',14);
					$this->setTextColorByName('black');;
					$img = NX_DOC_ROOT."nx/skins/lightsky/images/livret_fpdf/tele_livret.jpg";
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
					foreach ($offer->doctors as $doctor)
					{
						$doctor_name="Dr ".$doctor->first_name." ".strToUpper($doctor->last_name);
						$doctor_name = iconv('UTF-8', 'windows-1252', stripslashes($doctor_name));

						$doctor_cord="";
						if($doctor->phone != "")
						{
								$doctor_cord=" ".$doctor->phone;
						}
						if($doctor->phone2 != "")
						{
								$doctor_cord=$doctor_cord." ".$doctor->phone2;
						}

						$this->SetFont($this->page_font,'',8);
						$this->setTextColorByName('black');;
						$this->Cell(50,3,$doctor_name,0,0,'L');
						//$this->MultiCell(42,4,$doctor_name,"",1);
						if($doctor_cord != "")
							$this->Cell(1,4,":",0,0,'L');
						$this->Cell(30,4,$doctor_cord,0,1,'L');
					}
				}
			}
		}
	
	}

	function  getImageWidth($wImg,$hImg,$logo_y)
	{
		return ($wImg*$logo_y)/$hImg;
	}

	function setElementFont($element,$font)
	{
		if($font != "arial" && $font != "courier" && $font != "helvetica")
		{
			$this->AddFont($font,'');
		}
		$this->$element=$font;

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

	function AddPage() 
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
			$this->SetFillColor(20,7,0,4);
			$this->Rect(0,0, $pdf_larg, $pdf_long, F);
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
		$this->SetDrawColor(0,0,0,100);
		$this->setTextColorByName('black');;
		$this->SetFont($header_font,'',$header_size);
		$this->SetY($this->marginTop);
		if($this->page_type == "summary")
		{
			$this->Cell(0,4,$header_title,0,0,'C');
			$this->Ln(3);
			$this->SetLineWidth(1);
			$this->Cell(0,3,"","B",0,''); 
			$this->Ln(7);
		}
		if($this->page_type == "offer" || $this->page_type == "pole" || $this->page_type == "pôle" || $this->page_type == "note" || $this->page_type == "last_cover" )
		{
			$this->Cell(0,4,$header_title,0,0,'C');
			$this->Ln(3);
			$this->SetLineWidth(1);
			$this->Cell(0,3,"","B",0,'');
			$this->Ln(5);
		}
	}

	// Pied de page
	function Footer()
	{
		if($this->display_footer == true)
		{
			if($this->PageNo() == $this->num_pages["summary"])
			{
				$coord=$this->summaryInfos["summary_coord"];
				$this->setTextColorByName('black');;
				$this->SetFont($footer_font,'',10);
				//$this->SetFillColor(10,4,0,2);
				$this->SetFillColor(20,7,0,4);
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

				$this->SetLineWidth(1);
				$this->Cell(0,0,"","B",0,'');
			}
			elseif($this->PageNo() == $this->num_pages["emergency"])
			{
				$this->setTextColorByName('black');;
				$y=220-$this->marginBottom-$this->marginImp;
				$this->SetY($y);
				$this->SetFont($footer_font,'',8);
				$this->SetLineWidth(1.5);
				$this->Cell(0,0,"","B",0,'');
			}
			else
			{
				$this->setTextColorByName('black');;
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
	
}

?>

