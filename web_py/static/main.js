function mainf( Vue ) {
  

const app = new Vue({
  data: {
    maindata: [],
    colm_checked: ['svrid', 'svrname', 'clisock'],
    colm_base: ["svrid","svrname","clitype","clisock", // 主页里出现的栏目
        "_mainconf","shell","pid","rack","begin_time","atime"],
    sort_colm: "",
    sort_dir: 0, // 1 升序； -1 降序
    svrid_page: 0, // 大于0时 进详细页
    svrid_idx : 0,
    prvd_obj: [],
    invk_obj: {}
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
        const prvdKeys = ['regname', 'prvdid', 'url', 'protocol', 'weight', 'version', 'pvd_ok', 'pvd_ng', 'ivk_ok', 'ivk_ng']
        console.log("goto Detail svrid=" + svrid);
        this.svrid_page = svrid;
        this.svrid_idx = arridx;
        if (0 === svrid) return;

        let svrObj = this.maindata[arridx];
        this.prvd_obj = [];
        if ('_provider_mark' in svrObj) { // 服务提供者
            let prvdarr = svrObj['_provider_mark'].split('+');
            for (let prvdi in prvdarr) { // ['prvd_tt1-1', 'prvd_ff2-2']
                let prvdItemObj = {}; // { url: xx, regname: xx}
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

    refreshDetail : function(svridx) {
        let svrid = this.svrid_page;
        this.$http.get('/clidata/' + svrid).then(function(res){
            var result = res.body.code
            if (0 != result) {
                alert('获取失败：'+res.body.desc);
            } else {
                console.log(svrid + "更新详细成功：" + res.body.data);
                this.maindata[this.svrid_idx] = res.body.data;
            }
        }, function(){
            alert('请求/clidata/' + svrid + '失败处理');
        })
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
            'clisock':'IP',
            '_mainconf':'主配置',
            'shell':'启动命令',
            'pid':'进程ID',
            'rack':'机房机架',
            'begin_time':'启动时间',
            'atime':'活跃时间'
        }

        if (attrName in attrHumanObj){
            return attrHumanObj[attrName];
        }
        return attrName;
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