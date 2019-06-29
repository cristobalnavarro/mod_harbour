function Main()

  local n

  ?? '<body bgcolor="dodgerblue";>'

  ?? '<h2 style="color:white">Harbour > The Virtual Machine > The Global Symbol Table</h2>'

  for n = __DynsCount() to 1 step -1
     if __DynsIsFun( n )
        ?? '<p style="color:springgreen;">' + __DynsGetName( n ) + "</p>" + CRLF
     else
        ?? '<p style="color:yellow;">' + __DynsGetName( n ) + "</p>" + CRLF
     endif
  next

  ? "</body>"

return nil