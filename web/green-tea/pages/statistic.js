import Head from "next/head";
import Sidebar from "../components/Sidebar";
import StatisticContent from "../components/StatisticContent";
import Session from "./session";

export default function Statistic() {
    let page = (
        <div className="flex">
            <Head>
                <title>GreenTea Dashboard</title>
                <link rel="icon" href="/greentea.ico" />
            </Head>
            <Sidebar />
            <StatisticContent />
        </div>
    );

    return <Session page={page} />;
}
