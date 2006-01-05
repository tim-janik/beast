<?PHP error_reporting (E_ALL); // FIXME ?>

<div class="php-file-browser">

<?PHP
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
  function tdx ($nth, $flags, $p1 = -1, $p2 = -1, $p3 = -1, $p4 = -1) {
    $tag = sprintf ('<td class="php-file-browser-file%u"', $nth);
    if (strstr ($flags, 'W'))
      $tag .= ' nowrap';
    if (strstr ($flags, 'l'))
      $tag .= ' align="left"';
    if (strstr ($flags, 'c'))
      $tag .= ' align="center"';
    if (strstr ($flags, 'r'))
      $tag .= ' align="right"';
    if ($p4 != -1)	$tag .= sprintf (' style="padding: %.1fem %.1fem %.1fem %.1fem"', $p1, $p2, $p3, $p4);
    else if ($p3 != -1)	$tag .= sprintf (' style="padding: %.1fem %.1fem %.1fem"', $p1, $p2, $p3);
    else if ($p2 != -1)	$tag .= sprintf (' style="padding: %.1fem %.1fem"', $p1, $p2);
    else if ($p1 != -1)	$tag .= sprintf (' style="padding: %.1fem"', $p1);
    $tag .= '>';
    return $tag;
  }
  function td1 ($flags, $p1 = -1, $p2 = -1, $p3 = -1, $p4 = -1) { return tdx (1, $flags, $p1, $p2, $p3, $p4); }
  function td2 ($flags, $p1 = -1, $p2 = -1, $p3 = -1, $p4 = -1) { return tdx (2, $flags, $p1, $p2, $p3, $p4); }
  //
  //	file browser
  //
  @define ('PAGEURL', '');
  @define ('SRCDIR', './');
  @define ('SRCURL', '');
  @define ('PARENTMARK', '<b>&lt;</b>');
  @define ('CURRENTMARK', '<b>*</b>');
  @define ('DIRMARK', '<b>D</B>');
  @define ('FILEMARK', '&middot;');
  @define ('SOUNDMARK', '&middot;');
  @define ('ARCHIVEMARK', '&middot;');
  @define ('FILE0_STYLE', '');
  @define ('FILE1_STYLE', '');
  @define ('FILE2_STYLE', '');
  $loc = @$_GET['loc'];
  if (preg_match (',^(/[a-z_A-Z+0-9-]+)+/?$,', $loc)) {
    $trunk = $loc . '/';
    $absloc = SRCDIR . $trunk;
    $dir = opendir ($absloc);
  } else
    $dir = 0;
  if (!$dir) {
    $trunk = '/';
    $absloc = SRCDIR . $trunk;
    $dir = opendir ($absloc);
  }
  // trunk always starts and ends with '/'
  $now = time();
  $allfiles = array();
  for ($fname = readdir ($dir); $fname != false; $fname = readdir ($dir)) {
    $fpath = $absloc . $fname;
    $isfile = is_file ($fpath);
    if ((!$isfile && !is_dir ($fpath)) ||
        preg_match ('/\.DSC$/', $fname) ||
        ($trunk == '/' && $fname == '..'))
      continue;
    if ($fname == '.')
      $ftype = '1';
    else if ($fname == '..')
      $ftype = '0';
    else if ($fname[0] == '.')
      continue;
    else if ($isfile)
      $ftype = 'F';
    else
      $ftype = 'D';
    if ($isfile) {
      $sname = size_name (filesize ($fpath));
      $blurb = @file_get_contents ($fpath . '.DSC');
    } else {
      $sname = 'DIR';
      $blurb = @file_get_contents ($fpath . '/.dir.DSC');
    }
    $mtime = filemtime ($fpath);
    $date = date ("M d ", $mtime);
    if (abs ($mtime - $now) < 60 * 60 * 24 * 182)
      $date .= date ("H:i", $mtime);
    else
      $date .= date ("Y", $mtime);
    $allfiles[] = array ($ftype, $fname, $sname, $blurb, $date);
  }
  closedir ($dir); unset ($dir);
  function file_cmp ($ar1, $ar2) {
    $c0 = $ar1[0] < $ar2[0] ? -1 : $ar1[0] > $ar2[0];			// by type
    return $c0 ? $c0 : ($ar1[1] < $ar2[1] ? -1 : $ar1[1] > $ar2[1]);	// by name
  }
  usort ($allfiles, 'file_cmp');

  echo '<table cellspacing="3" border="0">', "\n";
  echo '<tr><td align="center" nowrap class="php-file-browser-file0" colspan="5" style="padding: 0.5em 1.0em">';
  printf ("<big><b>Directory: %s</b></big>\n", preg_replace ('/.*\/([^\/]+)\/$/', '\1', $trunk));
  echo "</td></tr>\n";
  $fcount = 0;
  for ($i = 0; $i < count ($allfiles); $i++) {
    $file = $allfiles[$i];
    $fname = $file[1];
    if ($file[0] == 'F') {
      $srcurl = SRCURL;
      $slen = strlen ($srcurl);
      if ($trunk[0] == '/' and ($slen == 0 or $srcurl[$slen - 1] == '/'))
        $tdir = substr ($trunk, 1); // skip initial '/'
      else
        $tdir = $trunk;
      $link = sprintf ('<a href="%s%s%s">', SRCURL, $tdir, $file[1]);
      $fcount++;
    } else if ($file[0] == '1')	// $fname == '.'
      $link = '';
    else {
      $link = sprintf ('%s?loc=%s%s', PAGEURL, $trunk, $file[1]);
      $link = preg_replace ('/\/\.$/', '', $link);			// remove /.
      $link = preg_replace ('/\/[a-z_A-Z+0-9-]+\/\.\.$/', '', $link);	// remove /xxx/..
      $link = preg_replace ('/\?loc=\/?\.?$/', '?loc=/', $link);	// canonicalize ?loc=[/][.]
      $link = '<a href="' . $link . '">';
    }
    $ca = strlen ($link) ? '</a' : '';
    if ($fname == '..')
      $fname = 'Parent Directory';
    else if ($fname == '.')
      $fname = 'Current Directory';
    $marks = array ('D' => DIRMARK, 'F' => FILEMARK, '1' => CURRENTMARK, '0' => PARENTMARK);
    if ($file[0] == 'F' && preg_match ('/\.(bse|mp3|ogg)$/i', $file[1]))
      $mark = SOUNDMARK;
    else if ($file[0] == 'F' && preg_match ('/\.(gz|tar\.gz|bz2|tar\.bz2|zip|tar|tgz|tbz)$/i', $file[1]))
      $mark = ARCHIVEMARK;
    else
      $mark = $marks[$file[0]];
    $szf = preg_match ('/DIR/', $file[2]) ? 'cW' : 'rW';
    $td = ($i & 1) ? 'td2' : 'td1';
    echo "<tr>";
    echo $td ('c', 0.1, 0.5, 0.1, 1),	$mark, "</td>\n";
    echo $td ('W', 0.3, 1),	'<b>', $link.$fname.$ca, "</b></td>\n";	// name
    echo $td ('', 0.3, 1),	       $file[3], "</td>\n";		// blurb
    echo $td ($szf, 0.3, 1),	'<b>', $file[2], "</b></td>\n";		// size
    echo $td ('W', 0.3, 1),	'<b>', $file[4], "</b></td>\n";		// date
    echo "</tr>\n";
  }
  echo '<tr><td align="right" nowrap class="php-file-browser-file0" colspan="5" style="padding: 0.5em 1.0em">';
  printf ("<small><b>%u Directories &middot; %u Files</b></small>", count ($allfiles) - $fcount, $fcount);
  echo "</td></tr>\n";
  echo "</table>\n";
?>

</div>
