;; -----------------------------------------------------------------------------------------
;;
;; Title      : Open Project Arduino
;;
;; Module     : open-project
;;
;; Description: Initialize Emacs as appropriate for working in this project
;;
;;              Copyright (c) 2008-2012 FEV GmbH, Germany.
;;
;;              All rights reserved. Reproduction in whole or in part is
;;              prohibited without the written consent of the copyright
;;              owner.
;;
;; -----------------------------------------------------------------------------------------

; Gobally define a reference to the base directory of this project. The
; definition is made relative to the directory where this Lisp script is
; found. The file needs therefore always be located in the cm folder in
; the root of the project's directory tree. 
(setq project-path (expand-file-name default-directory)
      project-path-win32 (subst-char-in-string ?/ ?\\ project-path nil)
)

; Load required stuff
(load-library "find-lisp-svn")

; TODO: Edit the list of directories holding source files
; Define the list of all source code directories
(defun list-of-source-dirs()
  "Return the list of directories (full pathes) which might contain source code files."
  (nconc
    ; Add particular directories to the following list
    (list "." "code")
    
    ; Here's a list of parent directories whose children and grand children are added to
    ; the search path.
    
    (fls-find-directories (concat project-path "code") ".")
    (fls-find-directories (getenv "ARDUINO_HOME") ".")
  )
) ; End of (defun list-of-source-dirs)




; Define a function for creation of a tags table for this project.
(defun create-tags-table()
  "The external program etags is called to make a new TAGS file for all relevant
C and M files in the scope of this project."
  (interactive)
  (let*
    (
      (list-of-directories (list-of-source-dirs))
      ; Do not change the path to the tags file: there's a reference to this path below.
      (tags-file (concat project-path-win32 "TAGS"))
      (del-command (concat "del " tags-file))
      ;(etags-command (concat "cd /D " project-path-win32 " & etags -a -o " tags-file))
      (etags-command (concat "cd /D " project-path-win32 " & etags -a -o " tags-file
                             " "
                     )
      )
      (operand-list "")
      (exec-command "")
      (exec-command-final "")
      (max-len-cmd-line 1000) ; Found by try and error
      (max-len-operand-list (- max-len-cmd-line (length etags-command)))
    )

    (print list-of-directories)
    (shell-command del-command)
    (while (not (null (car list-of-directories)))
      (let
        ((new-operands (concat " " (car list-of-directories) "/*.h"
                               " " (car list-of-directories) "/*.c"
                               " " (car list-of-directories) "/*.cpp"
                       )
         )
        )
        
        (if (< (+ (length operand-list) (length new-operands)) max-len-operand-list)
          ; The new files still fit into the command, extend the command line and next loop.
          (setq operand-list (concat operand-list new-operands)
                list-of-directories (cdr list-of-directories)
                run-etags (null (car list-of-directories))
          )
        
        ; else: Command line would become too long, submit command and start a new command
        ; line.
          (setq run-etags t)
          
        ) ; End (if Command line length not yet exceeded?)
        
        (if run-etags
          (progn
            (setq exec-command
                  (concat etags-command (subst-char-in-string ?/ ?\\ operand-list))
                  operand-list ""
            )
            (message exec-command)
            (shell-command exec-command)
          )
        ) ; End (if Command line complete for execution?)
      )
    )

    ; Load new tags file.
    (condition-case exc
      ; Protected code
      (visit-tags-table tags-file)
      ; Error handlers
      (error (message "Can't load TAGS file: %s" (car (cdr exc))))
    )
  )
) ; End of (defun create-tags-table)



(let
  (
  )

  ; Load the current tags table if available.
  (condition-case error-info
    (visit-tags-table project-path)
    ; Write a message if the previous expression signalled an error
    (error (message "TAGS couldn't be read %s" (cdr error-info)))
  )

  ; Configure ff-find-other-file
  (setq

    ; If non-nil, traces which directories are being searched.
    ff-quiet-mode nil

    ; Use other window to show the found include file.
    ff-always-in-other-window 't
    
    ; The regular expression and the file name extraction code for interpreting the
    ; #include lines.
    ;   We use a slightly modified copy of the original variable in find-file: Blanks in
    ; front of the #include are permitted.
    ff-special-constructs
      '(
        ;; C/C++ include, for NeXTSTEP too
        ("^\\s *\#\\s *\\(include\\|import\\)\\s +[<\"]\\(.*\\)[>\"]" .
         (lambda ()
           (setq fname (buffer-substring (match-beginning 2) (match-end 2)))))
        )
    
    ; ff-search-directories: List of directories searched for files with any extension
    ; specified in ff-other-file-alist.
    ;   Changes of ff-search-directories only affect the buffer's local copy of the value.
    ; The global default setting is cc-search-directories.
    cc-search-directories (list-of-source-dirs)

  ) ; End of (setq ff-find-other-file)

  ; List of directories to search for source files named in error messages. Elements should
  ; be directory names, not file names of directories. The value nil as an element means to
  ; try the default directory.
  ;   The location of the make file should always be in the list as files named in a
  ; compiler's output often referenced with relative paths from here.
  (setq compilation-search-path (list "." (concat project-path)))

  ; Prepare all frequently used build commands
  (let*
    ( (dsmake-cmd-base (concat "cd /d " project-path-win32 " & dsmake TEST_CASE=tc05 "))
      (make-cmd-base (concat "cd /d " project-path-win32 " & make -s APP=tc05 "))
      (doxygen-cmd-base (concat "cd /d " project-path-win32 "doc\\doxygen & "))
      (latex-cmd-base (concat "cd /d " project-path-win32 "doc\\manual & "))
      (history (list (concat make-cmd-base "download")
                     (concat make-cmd-base "bin\\DEBUG\\obj\\.o")
                     (concat make-cmd-base "build")
                     (concat make-cmd-base "rebuild")
                     (concat dsmake-cmd-base "rebuild")
                     (concat latex-cmd-base "make -s & manual.pdf")
                     (concat latex-cmd-base "make -s onePass")
                     (concat doxygen-cmd-base "doxygen doxyfile & start html\\index.html")
                     (concat doxygen-cmd-base "start html\\index.html")
               )
      )
    )

    (setq compile-history (append history compile-history nil)
          compile-command (car compile-history)
    )
  ) ; End of (let Prepare list of compile commands)


  ; Start interaction with user by switching to and presenting the most relevant source
  ; directory.
  (cd project-path)
  (find-file project-path)
  
) ; End of (let)
