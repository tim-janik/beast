#!/usr/bin/perl -W

$tag_handle = '@';
$doc_tag    = 'DOC';
$doc_input  = 'stdin';
$doc_output = 'stdout';

@doc_struct = (
  {
	name => {
	  start  => ".SH NAME\n#name# \\- #blurb#\n.SH SYNOPSIS\n",
	  repeat => 0,
	},

	synopsis => {
	  start  => "%s\n.br\n",
	  repeat => 0,
	},

	_prepend => ".TH \"#name#\" 3 \"#date#\" \"#package#\" \"#package#\" \n",

	_inline => {
	  nl     => "\n",
	  strong => "\\\\fB\$1\\\\fP",
	  emph   => "\\\\fI\$1\\\\fP",
	  ul     => "\\\\fU\$1\\\\fU",
	}
  },

  {
	name => {
	  start  => ".SH DESCRIPTION\n",
	  repeat => 0,
	},

	synopsis => {
	  start  => ".SS %s\n.PD 0\n",
	  repeat => 0,
	},

	parameter => {
	  start  => ".IP \\fI#type#\\ %s\\fP #parspace#\n",
	  repeat => 1,
	},

	pardesc => {
 	  start  => "%s\n",
	  repeat => 1,
	},

	returns => {
	  start  => ".IP \\fIRETURNS:\\fP #retspace#\n%s\n",
	  repeat => 1,
	},

	description => {
	  start  => ".PD 1\n.PP\n%s\n.PD\n",
	  repeat => 1,
	},

	_append  => "\n",

	_inline => {
	  nl     => "\n",
	  strong => "\\\\fB\$1\\\\fP",
	  emph   => "\\\\fI\$1\\\\fP",
	  ul     => "\\\\fU\$1\\\\fU",
	}
  }
);
