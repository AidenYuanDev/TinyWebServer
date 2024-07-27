window.onload = (even) => {

    //wrap 轮播图
    /*
    1、放图加absolute,让图片重叠在一起
    2、利用z-index把要显示的图片放在最前，
    3、添加事件，绑定按钮，每次点击就把.active类赋给他
    4、小圆点也是一样的，绑定事件，把相应的.active类赋给他
    */
    var forumBoxWrapItems = document.querySelectorAll('.forum-box-wrap-item');
    var goPreBtn = document.querySelector('.go-pre');
    var goNextBtn = document.querySelector('.go-next');
    var forumBoxWrapIndex = 0;
    var forumBoxWrapPoints = document.querySelectorAll('.forum-box-wrap-point');

    var clearForumBoxWrapActive = function () {
        for (var i = 0; i < forumBoxWrapItems.length; i++) {
            forumBoxWrapItems[i].className = 'forum-box-wrap-item';
        }
        for (var i = 0; i < forumBoxWrapPoints.length; i++) {
            forumBoxWrapPoints[i].className = 'forum-box-wrap-point';
        }
    }
    var goForumBoxWrapIndex = function () {
        clearForumBoxWrapActive();
        forumBoxWrapItems[forumBoxWrapIndex].classList.add('forum-box-wrap-active');
        forumBoxWrapPoints[forumBoxWrapIndex].classList.add('forum-box-wrap-point-active');
    }
    var goNext = function () {
        if (forumBoxWrapIndex < 4) forumBoxWrapIndex++;
        else forumBoxWrapIndex = 0;
        goForumBoxWrapIndex();
    }

    var goPre = function () {
        if (forumBoxWrapIndex > 0) forumBoxWrapIndex--;
        else forumBoxWrapIndex = 4;
        goForumBoxWrapIndex();
    }

    goNextBtn.addEventListener('click', function () {

        goNext();
    })

    goPreBtn.addEventListener('click', function () {
        goPre();
    })

    for (var i = 0; i < forumBoxWrapPoints.length; i++) {
        forumBoxWrapPoints[i].addEventListener('click', function () {
            forumBoxWrapPointIndex = this.getAttribute('data-index');
            forumBoxWrapIndex = forumBoxWrapPointIndex;
            goForumBoxWrapIndex();
        })
    }
    //关注
    var forumFocusOn = document.querySelector('.forum-focus-on');
    forumFocusOn.onclick = function () {
        if (forumFocusOn.style.color == 'black') {
            // forumFocusOn.style.color = 'LightSteelBlue';
            forumFocusOn.innerHTML = '已关注';
        }
        else {
            forumFocusOn.style.color = 'black';
            forumFocusOn.innerHTML = '+关注';
        }
    }
    // 论坛-图标-点赞、投币、收藏、转发
    var forumBoxHeaderIcon = document.querySelector('.forum-box-header-icon')
    var fIcon = forumBoxHeaderIcon.querySelectorAll('.icon');
    var flagIcon = new Boolean(0, 0, 0, 0);

    for (var i = 0; i < fIcon.length; i++) {
        fIcon[i].onclick = function () {
            if (this.style.color == '') this.style.color = 'FF8C00';
            else this.style.color = '';
        }
    }
    //导航栏隐藏
    var header = document.querySelector('.header');
    document.onscroll = function () {
        if (document.documentElement.scrollTop > 150) {
            header.style.display = 'none';
        }
        else header.style.display = 'block';
    }
    // 返回顶部
    var backTop = document.querySelector('.back-top');
    backTop.onclick = function () {
        var y = document.documentElement.scrollTop;
        var mytimer = setInterval(function () {
            y -= 20;
            document.documentElement.scrollTop = y;
            if (y <= 0) {
                clearInterval(mytimer);
            }
        }, 1)
    }

    window.onscroll = function () {
        if (document.documentElement.scrollTop > 300) backTop.style.display = 'block';
        else backTop.style.display = 'none';
    }
    //tab栏切换//
    var tab_list = document.querySelector('.tab-list');
    var lis = tab_list.querySelectorAll('li')
    var items = document.querySelectorAll('.item');

    for (var i = 2; i < lis.length; i++) {
        lis[i].setAttribute('index', i - 2);//设置索引号

        lis[i].onclick = function () {
            for (var i = 0; i < lis.length; i++) {
                lis[i].className = '';
            }

            this.className = 'current';
            var index = this.getAttribute('index');
            for (var i = 0; i < items.length; i++) {
                items[i].style.display = 'none';
            }
            // console.log(index);
            items[index].style.display = 'block';
        }
    }
    // 搜索栏
    const searchBox = document.querySelector(".search-box");
    const searchBtn = document.querySelector(".search-icon");
    const cancelBtn = document.querySelector(".cancel-icon");
    const searchInput = searchBox.querySelector("input");
    const searchData = document.querySelector(".search-data");
    searchBtn.onclick = () => {
        searchBox.classList.add("active");
        searchBtn.classList.add("active");
        searchInput.classList.add("active");
        cancelBtn.classList.add("active");
        searchInput.focus();
        if (searchInput.value != "") {
            var values = searchInput.value;
            searchData.classList.remove("active");
            searchData.innerHTML = "你刚刚搜索了     " + "<span style='font-weight: 500;'>" + values + "</span>";
        } else {
            searchData.textContent = "";
        }
    }
    cancelBtn.onclick = () => {
        searchBox.classList.remove("active");
        searchBtn.classList.remove("active");
        searchInput.classList.remove("active");
        cancelBtn.classList.remove("active");
        searchData.classList.toggle("active");
        searchInput.value = "";
    }

    //鼠标点击爱心
    !function (e, t, a) {
        function r() {
            for (var e = 0; e < s.length; e++)
                s[e].alpha <= 0 ? (t.body.removeChild(s[e].el), s.splice(e, 1)) : (s[e].y--, s[e].scale += .004, s[e].alpha -= .013, s[e].el.style.cssText = "left:" + s[e].x + "px;top:" + s[e].y + "px;opacity:" + s[e].alpha + ";transform:scale(" + s[e].scale + "," + s[e].scale + ") rotate(45deg);background:" + s[e].color + ";z-index:99999");
            requestAnimationFrame(r)
        }

        function n() {
            var t = "function" == typeof e.onclick && e.onclick;

            e.onclick = function (e) {
                t && t(),
                    o(e)
            }
        }

        function o(e) {
            var a = t.createElement("div");

            a.className = "heart",
                s.push({
                    el: a,
                    x: e.clientX - 5,
                    y: e.clientY - 5,
                    scale: 1,
                    alpha: 1,
                    color: c()
                }),
                t.body.appendChild(a)
        }

        function i(e) {
            var a = t.createElement("style");
            a.type = "text/css";

            try {
                a.appendChild(t.createTextNode(e))
            }

            catch (t) {
                a.styleSheet.cssText = e
            }

            t.getElementsByTagName("head")[0].appendChild(a)
        }

        function c() {
            return "rgb(" + ~~(255 * Math.random()) + "," + ~~(255 * Math.random()) + "," + ~~(255 * Math.random()) + ")"
        }

        var s = [];

        e.requestAnimationFrame = e.requestAnimationFrame || e.webkitRequestAnimationFrame || e.mozRequestAnimationFrame || e.oRequestAnimationFrame || e.msRequestAnimationFrame || function (e) {
            setTimeout(e, 1e3 / 60)
        }

            ,
            i(".heart{width: 10px;height: 10px;position: fixed;background: #f00;transform: rotate(45deg);-webkit-transform: rotate(45deg);-moz-transform: rotate(45deg);}.heart:after,.heart:before{content: '';width: inherit;height: inherit;background: inherit;border-radius: 50%;-webkit-border-radius: 50%;-moz-border-radius: 50%;position: fixed;}.heart:after{top: -5px;}.heart:before{left: -5px;}"),
            n(),
            r()
    }

        (window, document);


}