;; Copyright (C) 2003 Tim Janik
;;
;; This software is provided "as is"; redistribution and modification
;; is permitted, provided that the following disclaimer is retained.
;;
;; This software is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
;; In no event shall the authors or contributors be liable for any
;; direct, indirect, incidental, special, exemplary, or consequential
;; damages (including, but not limited to, procurement of substitute
;; goods or services; loss of use, data, or profits; or business
;; interruption) however caused and on any theory of liability, whether
;; in contract, strict liability, or tort (including negligence or
;; otherwise) arising in any way out of the use of this software, even
;; if advised of the possibility of such damage.

;; (bse-script-register <func> <options> <category> <blurb> <author> <license> ARGS...)
(bse-script-register 'modules2grid
		     ""
                     "/SNet/Grid Align"
		     (string-append "Round module positions to their nearest grid position, "
				    "so to aligns all modules within a synthesis network.")
		     "Tim Janik"
		     "Provided \"as is\", WITHOUT ANY WARRANTY;"
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
