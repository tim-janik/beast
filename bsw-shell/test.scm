(bse-server-register-plugins bse-server)
(define project (bse-server-use-new-project bse-server "foo"))
(bse-project-restore-from-file project "../test/simple-loop.bse")
(bse-server-run-project bse-server project)

(while (bse-project-is-playing project)
       (display "still playing\n")
       (sleep 10))
; (bse-server-halt-project bse-server project)
