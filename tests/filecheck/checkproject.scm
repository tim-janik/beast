;; CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0/
;; -*- scheme -*-

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
	 (if (bse-error-test error)
	     (display (string-append file ": "
				     "failed to load project: "
				     (bse-error-blurb error)
				     "\n")))
	 ;; release the newly created project
	 (bse-item-unuse project)))
     (cdr (command-line)))
