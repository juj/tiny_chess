mergeInto(LibraryManager.library, {
  upload_unicode_char_to_texture__proxy: 'sync',
  upload_unicode_char_to_texture__sig: 'iii',
  upload_unicode_char_to_texture: function(unicodeChar, charSize, applyShadow) {
    var canvas = document.createElement('canvas');
    canvas.width = canvas.height = charSize;
//  document.body.appendChild(canvas); // Debugging
    var ctx = canvas.getContext('2d');
    ctx.fillStyle = 'black';
    ctx.globalCompositeOperator = 'copy';
    ctx.globalAlpha = 0;
    ctx.fillRect(0, 0, canvas.width, canvas.height);
    ctx.globalAlpha = 1;
    ctx.fillStyle = 'white';
    ctx.font = charSize + 'px Arial Unicode';
    if (applyShadow) {
      ctx.shadowColor = 'black';
      ctx.shadowOffsetX = 2;
      ctx.shadowOffsetY = 2;
      ctx.shadowBlur = 3;
      ctx.strokeStyle = 'gray';
      ctx.strokeText(String.fromCharCode(unicodeChar), 0, canvas.height-7);
    }
    ctx.fillText(String.fromCharCode(unicodeChar), 0, canvas.height-7);
    GLctx.pixelStorei(GLctx.UNPACK_FLIP_Y_WEBGL, true);
    GLctx.texImage2D(GLctx.TEXTURE_2D, 0, GLctx.RGBA, GLctx.RGBA, GLctx.UNSIGNED_BYTE, canvas);
    GLctx.pixelStorei(GLctx.UNPACK_FLIP_Y_WEBGL, false);
  }
});
