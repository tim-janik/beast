#!/usr/bin/perl -W

$tag_handle = '@';
$doc_tag    = 'FAQ';
$doc_header = 'faq-header.markup';
$doc_input  = 'stdin';
$doc_output = 'stdout';

@doc_struct = (
  {
	category => {
	  start   => "%c - %s\n",
	  end     => "\n",
	  repeat  => 0,
# 	  counter => 'Alpha',
	  'reset' => 'title'
	},

	title => {
	  start   => "%category-c.%2c - %0-7p\n",
	  repeat  => 1,
	},


    _prepend => "FAQ Revised: " . localtime() . "\n" x 3 . "Table Of Contents\n\n",
	_append  => "\n\n"
  },

  {
	category => {
	  start   => "%c - %s\n",
	  end     => "\n\n",
	  repeat  => 0,
# 	  counter => 'Alpha',
	  'reset' => 'title'
	},

	title => {
	  start   => "\n%category-c.%2c - %p\n\n",
	  repeat  => 1
	},

	answer => {
	  start   => "%p\n\n",
	  repeat  => 1
	},

	_inline => {
	  nl     => "\n",
	  url    => "\$2 (\$1)",
	  mail   => "\$2 (\$1)",
	  strong => "*\$1*",
	  emph   => "_\$1_",
	}
  }
);
