import Head from "next/head";
import Sidebar from "../components/Sidebar";
import StatisticContent from "../components/StatisticContent";


export default function Statistic(){
    return(
        <div className="flex">
            <Head>
                <title>GreenTea Dashboard</title>
                <link rel="icon" href="/greentea.ico" />
            </Head>
            <Sidebar/>
            <StatisticContent />

        </div>
    )
}