

(load "/usr/share/common-lisp/source/slime/swank-loader.lisp")
(swank-loader:init)
(swank:create-server :port 4005)
;(sleep 10)
(print "Started swank")

