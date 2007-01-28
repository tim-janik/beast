<?php
  // Copyright 2005-2006 Tim Janik
  // GNU Lesser General Public License version 2 or any later version. 
  //
?>

<?PHP error_reporting (E_ALL); // FIXME ?>

<div class="php-file-upload">

<?PHP
  //
  //	presentation of "file upload" form
  //
  @define ('DESTINATIONDIR', '/tmp/file-uploads');
  @define ('NAME_TIP', '');
  @define ('EMAIL_TIP', '');
  @define ('DESCRIPTION_TIP', '');
  @define ('FILE_TIP', '');
  if ($_SERVER['REQUEST_METHOD'] != 'POST') { ?>

<center>
  <form method="POST" action="#POST" enctype="multipart/form-data">
    <table border="0" summary="File upload form">
      <tr>
        <td><strong>Name:</strong></td>
        <td><input type="text" name="name" size="50" maxlength="120" value=""/></td>
        <td><em><?php echo NAME_TIP; ?></em></td>
      </tr>
      <tr>
        <td><strong>Email:</strong></td>
        <td><input type="text" name="email" size="50" maxlength="50" value=""/></td>
        <td><em><?php echo EMAIL_TIP; ?></em></td>
      </tr>
      <tr>
        <td><strong>Description: &nbsp; </strong></td>
        <td><textarea name="desc" rows="10" cols="50"/></textarea></td>
        <td><em><?php echo DESCRIPTION_TIP; ?></em></td>
      </tr>
      <tr>
        <td><strong>File:</strong></td>
        <td><input type="hidden" name="MAX_FILE_SIZE" value="8388608"/>
            <input type="file" name="file1" size="40" value=""/></td>
        <td><em><?php echo FILE_TIP; ?></em></td>
      </tr>
      <tr>
        <td></td>
        <td valign="bottom" align="center" style="height: 4em">
            <input type="submit" value="      Send File      "/></td>
        <td></td>
      </tr>
    </table>
  </form>
</center>

<?PHP } ?>
    

<?PHP
  //
  //	file upload handling
  //
  if ($_SERVER['REQUEST_METHOD'] == 'POST') {
    $file = @$_FILES['file1'];
    $ferror = $file['error'];
    $fsize = $file['size'];
    $realname = $file['name'];
    $realname = preg_replace ('/[^-)a-z_A-Z+0-9.=$(]/', '_', $realname);
    $tmpname = $file['tmp_name'];
    $username = @$_POST['name'];
    $email = @$_POST['email'];
    $blurb = @$_POST['desc'];
    $errmsg = 0;
    //
    //	error checking
    //
    if (!$_FILES || !$_POST)	// file or memory limits were exceeded
      $ferror = 1;
    else if (!preg_match ("/\w+/", $username))
      $errmsg = "No user name supplied";
    else if (!preg_match ("/\w+.*@.*\.\w+/", $email))
      $errmsg = "No valid email adress supplied";
    else if (!preg_match ("/\w/", $blurb))
      $errmsg = "No description supplied";
    else if ($realname && !preg_match ('/\.(bse|ogg)$/i', $realname))
      $errmsg = "Invalid file type";
    else if (!$file || $ferror || $fsize <= 0 || $fsize > 8388608 || !$realname)
      $errmsg = "No data could be received."; // default error message
    //
    //  head room above messages
    //
    echo "<br><br><br><br>\n";
    //
    //	valid file upload
    //
    if (!$errmsg && !$ferror) {
      $destfile = DESTINATIONDIR . '/' . $realname;
      if (file_exists ($destfile)) {
        $counter = 1;
        while (file_exists ($destfile . '#' . $counter))
          $counter++;
        $destfile = $destfile . '#' . $counter;
      }
      $blurbfile = $destfile . ".DSC";
      if (!file_exists ($blurbfile)) {
        $text = sprintf ("%s <%s>\n%s\n", $username, $email, $blurb);
        $text_quoted = sprintf ("%s &lt;<a href=\"mailto:%s\">%s</a>&gt;<br>\n%s\n",
                                htmlspecialchars ($username), htmlspecialchars ($email), htmlspecialchars ($email),
                                htmlspecialchars ($blurb));
        if (!function_exists ('file_put_contents')) {
          function file_put_contents ($filename, $data) {
            if (($h = @fopen ($filename, 'x')) == false) { return false; }
            if (($bytes = @fwrite ($h, $data)) == false) { unlink ($filename); return false; }
            if (@fclose ($h) == false)                   { unlink ($filename); return false; }
            return $bytes; 
          }
        }
        if (file_put_contents ($blurbfile, $text_quoted, LOCK_EX) > 0) {
          if (move_uploaded_file ($tmpname, $destfile)) {
            $notified = @mail ("timj@beast.gtk.org", "BEAST-UPLOAD: ".$realname,
                               "=== BEAST-UPLOAD ===\n\n".
                               "FILE:\n".$realname."\n\n".
                               "DEST:\n".$destfile."\n\n".
                               "BLURB:\n" . $text . "\n");
            //
            //  all done, OK
            //
            echo "<center>\n";
            echo "<br>\n";
            echo '<big><b><u>File upload completed</u></b></big>', "<br>\n";
            echo "<br>Thank you for submitting: " . $realname;
            echo "<br>\n<br>";
            echo "Your file is currently being quarantined and will be added to <br>".
                 "the archive after investigation by the archive moderator.";
            if (!$notified)
              echo "<br><small>Warning: Failed internal notification for fast upload handling (ignored)</small>";
            echo "</center>\n";
          } else {
            unlink ($blurbfile);
            $errmsg = "Upload service temporarily unavailable (e901)";
          }
        } else
          $errmsg = "Upload service temporarily unavailable (e902)";
      } else
        $errmsg = "Upload service temporarily unavailable (e903)";
    }
    //
    //	error message handling
    //
    if ($errmsg || $ferror) {
      echo "<center>\n";
      echo "<br>\n";
      echo '<big style="color:#ff0000;"><b><u>File upload failed</u></b></big>', "<br>\n";
      if ($ferror) {
        $emsgs = array (0 => "OK",
			1 => "The file to upload was too big",			// PHP upload_max_filesize
			2 => "The file to upload was too big",			// form MAX_FILE_SIZE
                      	3 => "The file was only partially transfered", 		// partial upload
                      	4 => "No file contents were uploaded",			// no file was uploaded
		      	6 => "Upload service temporarily unavailable (6)",	// UPLOAD_ERR_NO_TMP_DIR
		      	7 => "Upload service temporarily unavailable (7)",	// UPLOAD_ERR_CANT_WRITE
		       );
	$errmsg = $emsgs[$ferror];
	if (!$errmsg)
	  $errmsg = sprintf ("Upload service temporarily unavailable (%u)", $ferror);
      } else if ($tmpname && $fsize == 0)
        $errmsg = sprintf ("File was empty or could not be transferred");
      printf ("<br>While attempting to receive uploaded file contents, the following error occoured:\n");
      printf ("<br>&nbsp; &nbsp; &nbsp; &nbsp; <b>%s</b><br>\n", $errmsg);
      echo "<br>\n";
      echo "Please restart the file upload after the described error has been corrected.<br>\n";
      echo "</center>\n";
    }
  }
?>

</div>
