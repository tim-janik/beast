(define project (bsw-server-use-new-project bsw-server "foo"))
(bsw-project-restore-from-file project "../test/simple-loop.bse")
(bsw-server-run-project bsw-server project)
; (bsw-server-halt-project bsw-server project)
(while (bsw-project-is-playing project)
  (display "still playing\n")
  (sleep 100))
  
