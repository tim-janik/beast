#!/usr/bin/perl -W

use strict;
use lib '../modules';
use Texitheque::CGI::FileBrowser;

my $browser = Texitheque::CGI::FileBrowser->new(
  -template     => '../html/browse-bse-files.html',
  -root_dir     => '../html/beast-ftp/bse-files',
  -web_base     => '../html',
  -mime_images  => '../html/css/images/mime',
  -default_mode => 'standard',
  -webmaster    => 'dirt@gtk.org',
);

$browser->run if defined $browser;
