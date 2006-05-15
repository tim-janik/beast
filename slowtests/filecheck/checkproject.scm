;; -*- scheme -*-
;; Copyright (C) 2006 Tim Janik
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

;; play argc/argv contents as bse files

;; load all stock BSE plugins
(bse-server-register-blocking bse-server-register-core-plugins #f)
;; no script registration
;;(bse-server-register-blocking bse-server-register-scripts #f)
;; no LADSPA plugin registration
;;(bse-server-register-blocking bse-server-register-ladspa-plugins #f)

;; for each file on the command line
(map (lambda (file)
       (let*
	   ;; create a new project
	   ((project (bse-server-use-new-project bse-server file))
	    ;; load file contents into project
	    (error (bse-project-restore-from-file project file)))
	 ;; check errors
	 (if (bse-test-error error)
	     (display (string-append file ": "
				     "failed to load project: "
				     (bse-error-blurb error)
				     "\n")))
	 ;; release the newly created project
	 (bse-item-unuse project)))
     (cdr (command-line)))
