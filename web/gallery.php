<!-- Copyright 2005-2006 Tim Janik
  -- GNU Lesser General Public License version 2 or any later version. 
  -->

<?PHP ini_set ('error_reporting', E_ALL); // FIXME ?>

<div class="php-gallery">

<?PHP
  //
  // common initialization
  //
  $images_per_page = 8; $images_per_line = 4; $page_url = ""; $maxlen = "80"; $range_span = 4; $thumb_width = 160;
  // read image info (one group per image)
  $images = parse_ini_file(GALLERY_FILE, TRUE) or die ("<br><b>Error</b>: Failed to read gallery definition");
  $images = array_values ($images); // ignore group names
  $n_pages = floor ((count ($images) + $images_per_page - 1) / $images_per_page);
  
  // figure requested page
  $page = max (1, min (intval (@$_GET['page']), $n_pages));
  $n_images = count ($images);
  $first_image = ($page - 1) * $images_per_page;
  $page_images = min ($images_per_page, $n_images - $first_image);
  
  // helper functions
  function size_name ($size) {
    $b = 1; $kb = 1024; $mb = $kb * $kb; $gb = $kb * $mb; $tb = $kb * $gb; $fmt = "%.1f";
    if      ($size / $b < 1000) { $size = $size / $b;  $sct = 'B'; $fmt = "%u"; }
    else if ($size / $kb < 100) { $size = $size / $kb; $sct = 'KB'; }
    else if ($size / $mb < 100) { $size = $size / $mb; $sct = 'MB'; }
    else if ($size / $gb < 100) { $size = $size / $gb; $sct = 'GB'; }
    else                        { $size = $size / $tb; $sct = 'TB'; }
    return sprintf ($fmt . " %s", $size, $sct);
  }
  function range_start ($pos, $min, $max) {
    global $range_span;
    $upper = min ($pos + $range_span, $max);
    return max ($upper - 2 * $range_span, $min);
  }
  function range_end ($pos, $min, $max) {
    global $range_span;
    $lower = range_start ($pos, $min, $max);
    return min ($lower + 2 * $range_span, $max);
  }
?>

<center>

<?PHP
  //
  // markup image
  //
  if (@$_GET['image']) {

    // full size image
    for ($i = 0; $i < $n_images; $i++) {
      if ($images[$i]['fname'] == $_GET['image']) {
        $image = $images[$i];
        $img_page = floor ($i / $images_per_page) + 1;
        printf ('<a href="%s?page=%u"><img src="%s" width="%u" height="%u" alt="%s"></a><br>',
                $page_url, $img_page, $image['fname'], $image['width'], $image['height'], basename ($image['fname']));
        $blurb = @$image['blurb'];
        if ($blurb) {
          echo '<table border="0">';
          printf ('<tr><td align="center"> %u x %u - %s </td></tr>',
                  $image['width'], $image['height'], size_name ($image['bytes']));
          printf ('<tr><td align="left" width="%u"><br>', $image['width']);	// MSIE6 would recursively apply "center" here
          echo $blurb, "</td></tr></table>";
        }
        break;
      }
    }
    $current = $i + 1;

    // image links
    echo "<br>Images: &nbsp; <b>";
    // link left
    if ($current > 1) {
      printf ('<a href="%s?image=%s">&lt;&lt;&lt;</a>', $page_url, $images[$current - 1 - 1]['fname']);
    } else {
      echo '&lt;&lt;&lt;';
    }
    // image numbers
    echo " &nbsp; ";
    $needdot = 0;
    for ($i = range_start ($current, 1, $n_images); $i <= range_end ($current, 1, $n_images); $i++) {
      if ($needdot++)
        echo " &middot; ";
      if ($i == $current) {
        printf (' %u ', $current);
      } else {
        printf (' <a href="%s?image=%s">%u</a> ', $page_url, $images[$i - 1]['fname'], $i);
        echo "\n";
      }
    }
    // link right
    echo " &nbsp; \n";
    if ($current < $n_images) {
      printf ('<a href="%s?image=%s">&gt;&gt;&gt;</a>', $page_url, $images[$current - 1 + 1]['fname']);
    } else {
      echo '&gt;&gt;&gt;';
    }
    echo "</b>\n";
  }
?>
  
<?PHP
  //
  // markup gallerie
  //
  if (!@$_GET['image']) {
    // display all images
    echo '<table cellspacing="10" cellpadding="0">';
    $cellsize = $thumb_width + 2 * 12 + 2 * 2; // image pixels + 2 * padding + 2 * border-width
    for ($j = $first_image; $j < $first_image + $page_images; $j += $images_per_line) {
      echo "<tr>\n";
      $k = min ($j + $images_per_line, $first_image + $page_images);
      for ($i = $j; $i < $k; $i++) {
        // add framed thumbnail
        $image = $images[$i];
        $anchor = sprintf ('<a href="%s?image=%s">', $page_url, $image['fname']);
        printf ('<td align="center" valign="top" width="%u" height="%u">' . $anchor . '<img src="%s" width="%u" height="%u" alt="%s">',
                $image['twidth'], $image['theight'], $image['tname'], $image['twidth'], $image['theight'], basename ($image['fname']));
        printf ('</a><br><small>%u x %u - %s</small>', $image['width'], $image['height'], size_name ($image['bytes']));
        echo '</td>', "\n";
      }
      echo "</tr><tr>\n";
      for ($i = $j; $i < $k; $i++) {
        $image = $images[$i];
        printf ('<td align="left" valign="top" width="%u">', $cellsize);	// MSIE6 would recursively apply "center" here
	$blurb = @$image['blurb'];
        if ($blurb) {
          if (strlen ($blurb) > $maxlen)
            $blurb = substr ($blurb, 0, $maxlen) . '...';
          echo $blurb;
	}
        echo "</td>\n";
      }
      echo "</tr>";
    }
    echo "</table>\n";
 
    // page links
    echo "<br>Pages: &nbsp; <b>";
    // page link left
    if ($page > 1) {
      printf ('<a href="%s?page=%u">&lt;&lt;&lt;</a>', $page_url, $page - 1);
    } else {
      echo '&lt;&lt;&lt;';
    }
    echo " &nbsp; ";
    // page numbers
    for ($i = 1; $i <= $n_pages; $i++) {
      if ($i > 1)
        echo " &middot; ";
      if ($i == $page) {
        printf (' [%u] ', $i);
      } else {
        printf (' <a href="%s?page=%u">[%u]</a> ', $page_url, $i, $i);
        echo "\n";
      }
    }
    echo " &nbsp; \n";
    // page link right
    if ($page < $n_pages) {
      printf ('<a href="%s?page=%u">&gt;&gt;&gt;</a>', $page_url, $page + 1);
    } else {
      echo '&gt;&gt;&gt;';
    }
    echo "</b>\n";
  }
?>

<?PHP // print_r ($GLOBALS); ?>

</center>

</div>
