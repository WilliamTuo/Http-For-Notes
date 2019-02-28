
// 找到要插入的位置的后一个元素
var x = document.getElementById("head");
var y = x.getElementsByTagName("/ul");

<script>
var para = document.createElement("li");
var node = document.createTextNode("插入元素");
para.appendChild(node);

var element = document.getElementById("head");
element.appendChild(para);
</script>

//  删除一个 HTML 元素
<script>
  var parent=document.getElementById("div1");
  var child=document.getElementById("p1");
  parent.removeChild(child);
</script>
