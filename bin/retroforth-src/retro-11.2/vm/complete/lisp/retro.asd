; Copyright (c) 2011, Oleksandr Kozachuk

(defsystem :retro
           :version "0.1"
           :author "Oleksandr Kozachuk <ddeus.forth@mailnull.com>"
           :depends-on (#+ecl :cffi)
           :components ((:file "package")
                        (:file "ngaro" :depends-on ("package"))))
