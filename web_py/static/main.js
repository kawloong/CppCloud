var vueapp = null;

function b64EncodeUnicode(str) {
    return btoa(encodeURIComponent(str).replace(/%([0-9A-F]{2})/g, function(match, p1) {
        return String.fromCharCode('0x' + p1);
    }));
}

function b64DecodeUnicode(str) {
    return decodeURIComponent(atob(str).split('').map(function(c) {
        return '%' + ('00' + c.charCodeAt(0).toString(16)).slice(-2);
    }).join(''));
}


function mainf( Vue ) {
  

vueapp = new Vue({
  data: {
    maindata: [],
    colm_checked: ['svrid', 'svrname', 'clisock'],
    colm_base: ["svrid","svrname","clitype","clisock", // 主页里出现的栏目
        "_mainconf","shell","pid","rack","begin_time","atime"],
    sort_colm: "",
    sort_dir: 0, // 1 升序； -1 降序
    svrid_page: 0, // 大于0时 进详细页
    svrid_idx : 0,
    detail_obj: {},
    prvd_obj: [],
    invk_obj: {},
    outtext: ''
  },

  methods: {
    onSort : function(column, isNum) {
        var dir = this.sort_colm === column ? -1*this.sort_dir : 1;
        this.maindata.sort(function(left, right){
            left = isNum? Number(left[column]): left[column];
            right = isNum? Number(right[column]): right[column];
            console.log("left="+left+" right="+right);
            var ret = (left < right) ? -1 : (left > right ? 1 : 0);
            return ret * dir;
        });

        this.sort_colm = column;
        this.sort_dir = dir;
        console.log(this.sort_colm, dir);
    },
    get_maindata : function(id) {
        this.$http.get('/clidata/' + id).then(function(res){
            var result = res.body.code
            if (0 != result) {
                alert('获取失败：'+res.body.desc);
            } else {
                this.maindata = res.body.data;
            }
        }, function(){
            alert('请求/svrall失败处理');
        })
    },

    isReadOnly : function(key){
        const readonlykey = {'aliasname': 1, 'desc': 1};
        return !(key in readonlykey);
    },

    spanclass : function(column) {
        let iconClass = "glyphicon-sort";
        if (column === this.sort_colm) {
            iconClass = 1 === this.sort_dir ? "glyphicon-arrow-up" : "glyphicon-arrow-down";
        }
        let retClass = {"glyphicon" : true};
        retClass[iconClass] = true;
        //console.log("iconClass="+iconClass);
        return retClass;
    },

    gotoDetail : function(svrid, arridx) {
        const prvdKeys = ['regname', 'prvdid', 'url', 'protocol', 'weight', 
            'version', 'pvd_ok', 'pvd_ng', 'ivk_ok', 'ivk_ng', 'enable']
        console.log("goto Detail svrid=" + svrid);
        this.svrid_page = svrid;
        this.svrid_idx = arridx;
        if (0 === svrid) return;

        this.detail_obj = this.maindata[arridx];
        if (! ('aliasname' in this.detail_obj)) {
            this.detail_obj['aliasname'] = ''
        }

        let svrObj = this.detail_obj;
        this.prvd_obj = [];
        if ('_provider_mark' in svrObj) { // 服务提供者
            let prvdarr = svrObj['_provider_mark'].split('+');
            for (let prvdi in prvdarr) { // ['prvd_tt1-1', 'prvd_ff2-2']
                let prvdItemObj = {}; // { url: xx, weight: xx}
                prvdItemObj.regname = prvdarr[prvdi];
                for (let idx in prvdKeys) {
                    let prvd_keyi = prvdarr[prvdi] + ':' + prvdKeys[idx];
                    //console.log("Prvd: " + prvd_keyi);
                    if ( prvd_keyi in svrObj ){
                        prvdItemObj[prvdKeys[idx]] = svrObj[prvd_keyi];
                    }
                }
                
                this.prvd_obj.push(prvdItemObj);
            }
        }

        // 服务消息者判断
        this.invk_obj = {};
        let prvdMarkstr = svrObj['_provider_mark'];
        for (let key in svrObj) {
            let value  = svrObj[key];
            if (0 === key.indexOf('prvd_')) {
                let invkArr = key.split(':');
                if (2 != invkArr.length) {
                    console.log('error prop-key ' + key);
                    continue;
                }
                if (prvdMarkstr) { // 属于提供者
                    if (prvdMarkstr.indexOf(invkArr[0]) >= 0) continue;
                }

                if (!this.invk_obj.hasOwnProperty(invkArr[0])){
                    this.invk_obj[invkArr[0]] = {};
                }
                this.invk_obj[invkArr[0]][invkArr[1]] = value;
            }
        }
    },
    
    propChange : function(param, key, val0){
        console.log("change prop " + param.target.value);
        if (val0 == param.target.value) return;
        
        if ('aliasname' === key){
            const url = '/notify/aliasname';
            this.$http.get(url, {params:{
                svrid: this.svrid_page,
                aliasname: param.target.value
            }}).then(function(resp){
                var result = resp.body.code
                if (0 != result){
                    console.log("reset to " + val0);
                }
            }, function(){
                console.log("reset to " + val0);
            })
        }
    },

    refreshDetail : function(svridx) {
        let self = this;
        let svrid = this.svrid_page;

        self.$data.outtext += '刷新信息：';
        this.$http.get('/clidata/' + svrid).then(function(res){
            var result = res.body.code
            if (0 != result) {
                self.$data.outtext += ('获取失败：'+res.body.desc + '\n');
            } else {

                console.log(svrid + "更新详细成功：" + res.body.data[0]);
                this.detail_obj = res.body.data[0];
                this.maindata[this.svrid_idx] = this.detail_obj;

                this.gotoDetail(svrid, this.svrid_idx);
                self.$data.outtext += ' 成功svrid=' + svrid + '\n';
            }
        }, function(){
            self.$data.outtext += ('请求/clidata/' + svrid + '失败处理\n');
        })
    },

    checkAlive : function() {
        let self = this;
        let url = '/notify/check-alive';
        self.$data.outtext += '检查存活：';
        this.$http.get(url, {params:{svrid: this.svrid_page}} 
                ).then(function(res) {
                self.ajaxCallBack(url, res, function(body){
                    // console.log('response' + this.outtext + body.result);
                    self.$data.outtext += ' 成功' + body.result + '\n';
                });
            }, function(){
                self.ajaxCallBack(url, false, null);
        }
        )
    },

    clickClose: function(idx){
        let self = this;
        let url = 2===idx? '/notify/closelink' :'/notify/exit';
        const cmdarr = ['通知关闭', '强制关闭', '断开连接'];

        self.$data.outtext += cmdarr[idx] + ":";
        this.$http.get(url, {params:{svrid: this.svrid_page, force: idx}} 
            ).then(function(res) {
            self.ajaxCallBack(url, res, function(body){
                // console.log('response' + this.outtext + body.result);
                self.$data.outtext += ' 成功' + body.result + '\n';
                self.$data.outtext += '关闭成功后，将不能进行其他操作，请重新从服务主页开始\n';
            });
        }, function(){
            self.ajaxCallBack(url, false, null);
        })
    },
    
    checkCpuInfo: function(idx){
        let self = this;
        let url = '/notify/shellcmd';
        const cmdarr = ['x', '获取CPU信息', '获取MEMERY'];

        self.$data.outtext += cmdarr[idx] + ":";
        this.$http.get(url, {params:{svrid: this.svrid_page, cmdid: idx}} 
            ).then(function(res) {
            self.ajaxCallBack(url, res, function(body){
                // console.log('response' + this.outtext + body.result);
                self.$data.outtext += ' 成功\n  ' + b64DecodeUnicode(body.result) + '\n';
            });
        }, function(){
            self.ajaxCallBack(url, false, null);
        })
    },

    checkIOStat: function(){
        let self = this;
        let url = '/notify/iostat';
        
        self.$data.outtext += "IO统计:";
        this.$http.get(url, {params:{ svrid: this.svrid_page }} 
            ).then(function(res) {
            self.ajaxCallBack(url, res, function(body){
                console.log('response' + body.result);

                let allstat = body.result.all;
                self.$data.outtext += ' 成功 ' + 
                'recv ' + allstat[0] + 'bytes@' + allstat[2] + 'package' +
                 ', send ' + allstat[1] + 'bytes@' + allstat[3] + 'package'
                 + '\n';
            });
        }, function(){
            self.ajaxCallBack(url, false, null);
        })
    },

    ajaxCallBack : function(url, res, okcb) {
        if (res) {
            var result = res.body.code
            if (0 === result) {
                okcb(res.body);
            } else {
                //alert('返回失败：'+res.body);
                this.$data.outtext += '  返回失败：' + JSON.stringify(res.body) + '\n';
            }
        } else {
            //alert('请求' + url + '失败处理');
            this.$data.outtext += '请求' + url + '失败处理' + '\n'; 
        }
    }

  },

  filters : {
      toHumanTime : function(val) {
        var d = new Date(val * 1000); 
        var date = (d.getFullYear()) + "-" + 
                   (d.getMonth() + 1) + "-" +
                   (d.getDate()) + " " + 
                   (d.getHours()) + ":" + 
                   (d.getMinutes()) + ":" + 
                   (d.getSeconds());
        return date;
      },

    // 属性变量名转可读中文词
    attrName2Human : function(attrName) {
        const attrHumanObj = {
            'svrid': '客户ID',
            'svrname':'应用名',
            'clitype':'应用类型',
            'clisock':'远端地址',
            '_mainconf':'主配置',
            'shell':'启动命令',
            'pid':'进程ID',
            'rack':'机房机架',
            'begin_time':'启动时间',
            'atime':'活跃时间',
            'localsock': '本地地址',
            '_provider_mark': '服务标记',
            'fd': '套接字fd',
            'desc': '描述'
        }

        if (attrName in attrHumanObj){
            return attrHumanObj[attrName];
        }
        return attrName;
    },

    // 属性值转可读
    value2Human : function(val, attrName) {
        const timeAttrMap = {'atime':1, 'begin_time':1};
        if (attrName in timeAttrMap){
            return vueapp.$options.filters.toHumanTime(val);
        }
        return val;
    },

    filterNullStr : function(param) {
        return param? param: 0;
    }
  },

  created: function() {
    this.get_maindata(0);
  }

  
}).$mount('#app')

}

mainf(Vue);