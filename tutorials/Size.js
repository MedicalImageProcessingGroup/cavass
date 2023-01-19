  // window resize - runs automatically when including page is loaded
  if (document.all)  {
    top.window.resizeTo(880,600);
  } //if
  else if (document.layers||document.getElementById)  {
    if (top.window.outerHeight<screen.availHeight||top.window.outerWidth<screen.availWidth)  {
      top.window.outerHeight = 600;
      top.window.outerWidth = 880;
    } //if
  } //else if

  // end window resize

