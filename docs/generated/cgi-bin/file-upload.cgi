#!/usr/bin/perl -W

use strict;
use lib '../modules';
use Texitheque::CGI::FileUploader;

my $uploader = Texitheque::CGI::FileUploader->new(
  -webmaster        => 'dirt@gtk.org',
  -template         => '../html/upload-bse-files.html',
  -contrib_queue    => '../contrib-queue',
  -moderators       => [ 'timj@gtk.org', 'dirt@gtk.org' ],
  -archive          => { 'browse-bse-files.html' => 'BEAST/BSE Files' },
  -max_post_size    => 10240, # KBytes
  -notify_subject   => 'BEAST-CONTRIB: File @file@ from @name@ <@email@>',
  -notify_body      => q{
  
  Name:        @name@
  EMail:       @email@
  Filename:    @file@
  Description: @desc@
  
  Approve contribution:
  mv -iv /web/beast/contrib-queue/@file@ /web/beast/contrib-queue/@file@.DSC /web/beast/contrib-queue/bse-files/
  
  Censor contribution:
  rm -iv /web/beast/contrib-queue/@file@ /web/beast/contrib-queue/@file@.DSC
},
  # Configurable messages:
  #   -notify_subject, -notify_body, -desc_file_body, -html_form
  # Available vars in messages: @name@ @email@ @desc@ @file@
);

$uploader->run if defined $uploader;
