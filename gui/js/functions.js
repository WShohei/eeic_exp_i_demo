const childProcess = require('child_process');
const psTree = require('ps-tree')


$(function () {
    var ip = "";
    var port = "";
    var shift_f = "";
    var cp_client = null;
    var started = false;

    $('#button-start').on('click', function () {
        if(started) {
            return
        }
        if(ip == ""){
            $("#std_output-area").html("ipアドレスを入力してください");
        } else if(port == ""){
            $("#std_output-area").html("ポートを入力してください");
        } else if(shift_f == ""){
            $("#std_output-area").html("シフト周波数を入力してください");
        } else {
            started = true
            //cp_client = childProcess.exec(`./js/src/run_client.sh ${ip} ${port}`, (error, stdout, stderr) => {
            console.log(__dirname)
            //rec --buffer 192 -t raw -b 16 -c 1 -e s -r 3000 - | ./build/multiclient $1 $2 $3 | play --buffer 192  -t raw -b 16 -c 1 -e s -r 3000 -
            //cp_client = childProcess.exec(`rec -t raw -b 16 -c 1 -e s -r 44100 - | ` + __dirname + `/js/src/clientsfu ${ip} ${port} ${shift_f} | play -t raw -b 16 -c 1 -e s -r 44100 -`, (error, stdout, stderr) => {
            cp_client = childProcess.exec(`rec --buffer 192 -t raw -b 16 -c 1 -e s -r 3000 - | /home/denjo/ドキュメント/GitHub/eeic_exp_i_demo/build/multiclient ${ip} ${port} ${shift_f} | play --buffer 192  -t raw -b 16 -c 1 -e s -r 3000 -`, (error, stdout, stderr) => {
                if(error) {
                    $("#std_output-area").html(error);
                    return
                }
            });
            $("#std_output-area").html("接続しました"); //嘘
        }
    });

    const cleanup = () => {
        try {
            psTree(cp_client.pid, (err, children) => {
                console.log(children)
                children.forEach((child) => {
                    process.kill(child.PID)
                })
            })
        } catch(e){

        }
    }
    process.on('SIGINT', cleanup)
    process.on('SIGTERM', cleanup)
    process.on('SIGQUIT', cleanup)
    process.on('uncaughtException', cleanup)
    //app.on('quit', cleanup) 本来はこれをよばないと子プロセスが消えない


    $('#button-dst_info').on('click', function () {
        ip = $("#input-dst_ip").val().toString();
        port = $("#input-dst_port").val().toString();
        shift_f = $("#input-shift_f").val().toString();
        $("#dst-info").html(`<span>IP : ${ip != "" ? ip : "***.***.***.***"}</span><span>PORT : ${port != "" ? port : "*****"}</span><span>シフト周波数 : ${shift_f != "" ? shift_f : "*****"}</span>`)
    });

    $('#button-stop').on('click', function () {
        if(started == false){
            return
        }
        started = false

        cleanup()
        $("#std_output-area").html("切断しました");
    });
})
