;; (bse-script-register <func> <category> <blurb> <help> <author> <copyright> ARGS...)

(bse-script-register 'modules2grid
                     "/SNet/Utils/Grid Align"
		     "Align modules to grid"
		     (string-append "This function rounds module positions to their nearest grid position "
				    "and thusly aligns all modules within a synthesis network.")
		     "Tim Janik (Author)"
		     "Copyright (C) 2003 Tim Janik"
		     (bse-param-snet "snet"))

(define (modules2grid snet)
  (if (not (bse-is-snet snet))
      (bse-script-exit 'error "no valid synthesis network supplied"))
  (bse-item-group-undo snet "modules2grid")      ; move all children at once
  (do ((children (bse-container-list-items snet) (cdr children)))
      ((null? children))
    (let ((module (car children)))
      (if (bse-is-source module)
	  (let ((x (bse-item-get module "pos-x"))
		(y (bse-item-get module "pos-y")))
	    (if #f (display (string-append (bse-item-get-name-or-type (car children))
					   ": " (number->string x)
					   ", " (number->string y)
					   "\n")))
	    (bse-source-set-pos module (round x) (round y))))))
  (bse-item-ungroup-undo snet))
