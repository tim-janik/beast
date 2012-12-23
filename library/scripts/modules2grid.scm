;; CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0/

;; (bse-script-register <func> <options> <category> <blurb> <author> <license> ARGS...)
(bse-script-register 'modules2grid
		     ""
                     (N_ "/SNet/Grid Align")
		     (N_ "Round module positions to their nearest grid position, "
			 "so to align all modules within a synthesis network.")
		     "Tim Janik"
		     "Provided \"as is\", WITHOUT ANY WARRANTY;"
		     (bse-param-snet (N_ "Synth Net")))

(define (modules2grid snet)
  (if (not (bse-is-snet snet))
      (bse-exit-error 'text1 (_"No valid synthesis network supplied")))
  (bse-item-group-undo snet "modules2grid")      ; move all children at once
  (do ((children (bse-container-list-children snet) (cdr children)))
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
