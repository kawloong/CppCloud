var vueapp = null;
function conffun( Vue ) {
    vueapp = new Vue({
        data: {
            filenames: {}, // {"f1": [isdel, mtime, contants, modifyfg, timehightlight]}
            selfile: '', // 选中的文件
            seltime: '',
            contents: ''
        },

        methods: {
            getFileName: function() {
                const url = "/confname";
                let self = this;
                this.$http.get(url).then(function(res){
                    if (res.body instanceof Object) {
                        self.filenames = res.body;
                        console.log(self.filenames.toString());
                    }else {
                        console.log('执行失败：' + JSON.stringify(res.body));
                    }
                }, function(){
                    console.error('请求' + url + '失败');
                })
            },

            getContanct: function(fname) {
                const url = '/getconf?key_pattern=/&file_pattern=' + fname;
                let self = this;
                this.$http.get(url).then(function(resp){
                    if (resp.body.code === 0) {
                        const filename = resp.body.file_pattern;
                        self.filenames[filename][1] =  resp.body.mtime;
                        self.filenames[filename][2] =  JSON.stringify(resp.body.contents);
                        self.filenames[filename][3] =  false; // 修改标记
                        self.filenames[filename][4] = '';

                        self.contents = JSON.stringify(resp.body.contents, null, 4);
                    } else {
                        console.error('执行失败: ' + JSON.stringify(res.body));
                    }
                }, function(){
                    console.error('请求' + url + '失败');
                })
            },

            chooseFile: function(fname){
                if (this.selfile && fname != this.selfile && this.filenames[this.selfile][3]) {
                    // 发生了修改时切换
                    // 暂存
                    this.filenames[this.selfile][2] = this.contents;
                }

                this.selfile = fname;
                this.seltime = this.filenames[fname][1];
                if (this.filenames[fname].length < 3) {
                    this.getContanct(fname);
                }

                this.contents = this.filenames[fname][2];
            },

            refresh: function(){
                if (!this.selfile) {
                    alert('no select file');
                    return;
                }
                if (this.filenames[this.selfile][3]){ // 修改过，提示
                    if (!confirm('文件:' + this.selfile + '修改过，确定继续会被覆盖')){
                        return;
                    }
                }

                this.getContanct(this.selfile);
            },

            modify: function(){ // 向后台提交修改
                if (!this.selfile) {
                    alert('no select file');
                    return;
                }

                const url = '/setconf';
                // json合法性检查
                if (!this.jsonValid(this.contents)){
                    alert("无效的json格式");
                    return;
                }
                if (!confirm('提交修改'+this.selfile+"?")){
                    return;
                }

                let self = this;
                let opFilename = this.selfile;
                this.filenames[opFilename][2] = this.contents;

                this.$http.post(url, {
                    filename: opFilename,
                    contents: JSON.parse(this.contents)
                }).then(function(resp){
                    if (resp.body.code === 0){
                        console.log('修改' + opFilename + '成功, ' + self.filenames[opFilename][3]);
                        self.filenames[opFilename][1] = resp.body.mtime;
                        //self.$set(self.filenames[opFilename], 3, false);
                        self.filenames[opFilename][3] = false;
                        self.$set(self.filenames[opFilename], 4, 'time_hl');
                        this.seltime = resp.body.mtime;
                    } else {
                        alert('响应失败:' + resp.body.desc);
                    }
                }, function(){
                    alert('POST失败' + url);
                });
            },

            inputChange: function(istrue){
                if (this.selfile in this.filenames){
                    this.filenames[this.selfile][3] = istrue;
                }
            },
            
            jsonValid: function(str) {
                if (typeof str == 'string') {
                    try {
                        var obj=JSON.parse(str);
                        return (typeof obj == 'object' && obj );
                    } catch(e) {
                        console.error('error：'+str+'!!!'+e);
                        return false;
                    }
                }
                console.log('It is not a string!')
            }
        },

        created: function() {
            this.getFileName();
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
            }
        }
    }).$mount('#app')
};

conffun(Vue);